#include "nocc.h"

#define control_flow_state_none 0
#define control_flow_state_break_bit 1
#define control_flow_state_continue_bit 2

void sema_push_scope(ParserContext *ctx) {
    assert(ctx != NULL);

    scope_stack_push(ctx->env);
    scope_stack_push(ctx->struct_env);
}

void sema_pop_scope(ParserContext *ctx) {
    assert(ctx != NULL);

    scope_stack_pop(ctx->struct_env);
    scope_stack_pop(ctx->env);
}

Vec *control_flow_state_new(void) {
    Vec *state;

    state = vec_new();
    vec_push(state, (void *)(intptr_t)control_flow_state_none);

    return state;
}

int control_flow_current_state(ParserContext *ctx) {
    assert(ctx != NULL);
    assert(ctx->flow_state->size > 0);

    return (intptr_t)vec_back(ctx->flow_state);
}

void control_flow_push_state(ParserContext *ctx, int flow_state) {
    assert(ctx != NULL);

    vec_push(ctx->flow_state,
             (void *)((intptr_t)flow_state | control_flow_current_state(ctx)));
}

void control_flow_pop_state(ParserContext *ctx) {
    assert(ctx != NULL);
    assert(ctx->flow_state->size > 1);

    vec_pop(ctx->flow_state);
}

bool is_break_accepted(ParserContext *ctx) {
    assert(ctx != NULL);
    return control_flow_current_state(ctx) & control_flow_state_break_bit;
}

bool is_continue_accepted(ParserContext *ctx) {
    assert(ctx != NULL);
    return control_flow_current_state(ctx) & control_flow_state_continue_bit;
}

ExprNode *implicit_cast_node_new(ExprNode *expr, Type *dest_type) {
    CastNode *p;

    assert(expr != NULL);
    assert(dest_type != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_cast;
    p->filename = str_dup(expr->filename);
    p->line = expr->line;
    p->type = dest_type;
    p->is_lvalue = false;
    p->operand = expr;

    return (ExprNode *)p;
}

bool can_cast_into(Type *src_type, Type *dest_type) {
    assert(src_type != NULL);
    assert(dest_type != NULL);

    switch (dest_type->kind) {
    case type_void:
        /* OK: T -> void */
        return true;

    case type_int8:
    case type_int32:
    case type_pointer:
        switch (src_type->kind) {
        case type_int8:
        case type_int32:
        case type_pointer:
            return true;

        default:
            return false;
        }

    default:
        return false;
    }
}

ExprNode *decay_type_conversion(ExprNode *expr) {
    assert(expr != NULL);

    if (is_array_type(expr->type)) {
        return implicit_cast_node_new(
            expr, pointer_type_new(array_element_type(expr->type)));
    }

    if (is_function_type(expr->type)) {
        return implicit_cast_node_new(expr, pointer_type_new(expr->type));
    }

    return expr;
}

ExprNode *integer_promotion(ExprNode *expr) {
    assert(expr != NULL);

    expr = decay_type_conversion(expr);

    if (is_int8_type(expr->type)) {
        return implicit_cast_node_new(expr, type_get_int32());
    }

    return expr;
}

void usual_arithmetic_conversion(ExprNode **left, ExprNode **right) {
    assert(left != NULL);
    assert(*left != NULL);
    assert(right != NULL);
    assert(*right != NULL);

    *left = integer_promotion(*left);
    *right = integer_promotion(*right);
}

bool relational_operation_type_conversion(ExprNode **left, ExprNode **right) {
    assert(left != NULL);
    assert(*left != NULL);
    assert(right != NULL);
    assert(*right != NULL);

    *left = integer_promotion(*left);
    *right = integer_promotion(*right);

    if (!is_scalar_type((*left)->type) || !is_scalar_type((*right)->type)) {
        return false;
    }

    if (is_pointer_type((*left)->type) && is_pointer_type((*right)->type)) {
        if (is_void_pointer_type((*left)->type) &&
            !is_void_pointer_type((*right)->type)) {
            /* void* < T* */
            *right = implicit_cast_node_new(*right, (*left)->type);
        } else if (!is_void_pointer_type((*left)->type) &&
                   is_void_pointer_type((*right)->type)) {
            /* T* < void* */
            *left = implicit_cast_node_new(*left, (*right)->type);
        }
    }

    return type_equals((*left)->type, (*right)->type);
}

bool assign_type_conversion(ExprNode **expr, Type *dest_type) {
    assert(expr != NULL);
    assert(*expr != NULL);
    assert(dest_type != NULL);

    *expr = decay_type_conversion(*expr);

    if (is_incomplete_type((*expr)->type) || is_incomplete_type(dest_type)) {
        return false;
    }

    if (type_equals((*expr)->type, dest_type)) {
        return true;
    }

    if (!can_cast_into((*expr)->type, dest_type)) {
        return false;
    }

    *expr = implicit_cast_node_new(*expr, dest_type);
    return true;
}

bool default_argument_promotion(ExprNode **expr) {
    assert(expr != NULL);
    assert(*expr != NULL);

    *expr = integer_promotion(*expr);

    switch ((*expr)->type->kind) {
    case type_int8:
    case type_int32:
    case type_pointer:
    case type_struct:
        return true;

    default:
        return false;
    }
}

Type *sema_identifier_type(ParserContext *ctx, const Token *t) {
    DeclNode *p;

    assert(ctx != NULL);
    assert(t != NULL);

    /* find symbol */
    p = scope_stack_find(ctx->env, t->text, true);

    if (p == NULL) {
        fprintf(stderr, "error at %s(%d): type %s not found in this scope\n",
                t->filename, t->line, t->text);
        exit(1);
    }

    /* check the symbol kind */
    if (p->kind != node_typedef) {
        fprintf(stderr, "error at %s(%d): symbol %s is not a type\n",
                t->filename, t->line, t->text);
        exit(1);
    }

    return ((TypedefNode *)p)->type;
}

MemberNode *sema_struct_member(ParserContext *ctx, Type *type, const Token *t) {
    MemberNode *p;

    assert(ctx != NULL);
    assert(type != NULL);
    assert(t != NULL);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_member;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->type = type;
    p->identifier = str_dup(t->text);
    p->generated_location = NULL;

    /* type check */
    if (is_incomplete_type(type)) {
        fprintf(stderr,
                "error at %s(%d): member of struct must be a complete type\n",
                p->filename, p->line);
        exit(1);
    }

    /* redefinition check */
    if (scope_stack_find(ctx->env, p->identifier, false)) {
        fprintf(stderr, "error at %s(%d): member %s is already defined\n",
                p->filename, p->line, p->identifier);
        exit(1);
    }

    /* register member symbol */
    scope_stack_register(ctx->env, p->identifier, p);

    return (MemberNode *)p;
}

StructType *sema_struct_type_register_or_new(ParserContext *ctx, const Token *t,
                                             const Token *identifier,
                                             bool search_recursively) {
    StructType *p;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(identifier != NULL);

    /* find type from symbol if exists */
    p = scope_stack_find(ctx->struct_env, identifier->text, search_recursively);

    if (p != NULL) {
        return p;
    }

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = type_struct;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->identifier = str_dup(identifier->text);
    p->members = NULL;
    p->num_members = 0;
    p->is_incomplete = true;
    p->generated_type = NULL;

    /* register struct type symbol */
    scope_stack_register(ctx->struct_env, p->identifier, p);

    return p;
}

Type *sema_struct_type_without_body(ParserContext *ctx, const Token *t,
                                    const Token *identifier) {
    return (Type *)sema_struct_type_register_or_new(ctx, t, identifier, true);
}

StructType *sema_struct_type_enter(ParserContext *ctx, const Token *t,
                                   const Token *identifier) {
    StructType *p;

    /* find type from symbol if exists */
    p = sema_struct_type_register_or_new(ctx, t, identifier, false);

    /* redefinition check */
    if (!p->is_incomplete) {
        if (p->identifier) {
            fprintf(stderr, "error at %s(%d): redefinition of struct %s\n",
                    t->filename, t->line, p->identifier);
        } else {
            /* TODO: refactoring with conditional expression */
            fprintf(stderr, "error at %s(%d): redefinition of struct %s\n",
                    t->filename, t->line, "<anonymous>");
        }
        exit(1);
    }

    /* enter struct member scope */
    /* but do not push struct scope */
    scope_stack_push(ctx->env);

    return p;
}

Type *sema_struct_type_leave(ParserContext *ctx, StructType *type,
                             MemberNode **members, int num_members) {
    int i;

    assert(ctx != NULL);
    assert(type != NULL);
    assert(type->is_incomplete);
    assert(members != NULL || num_members == 0);
    assert(num_members >= 0);

    /* leave struct member scope */
    scope_stack_pop(ctx->env);

    /* check the number of members */
    if (num_members == 0) {
        fprintf(stderr, "error at %s(%d): empty struct is not supported\n",
                type->filename, type->line);
        exit(1);
    }

    /* fix struct type */
    type->members = malloc(sizeof(MemberNode *) * num_members);
    type->num_members = num_members;
    type->is_incomplete = false;

    for (i = 0; i < num_members; i++) {
        type->members[i] = members[i];
    }

    return (Type *)type;
}

ExprNode *sema_paren_expr(ParserContext *ctx, const Token *open, ExprNode *expr,
                          const Token *close) {
    assert(ctx != NULL);
    assert(open != NULL);
    assert(expr != NULL);
    assert(close != NULL);

    return expr;
}

ExprNode *sema_integer_expr(ParserContext *ctx, const Token *t, int value) {
    IntegerNode *p;

    assert(ctx != NULL);
    assert(t != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_integer;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->type = type_get_int32();
    p->is_lvalue = false;
    p->value = value;

    return (ExprNode *)p;
}

ExprNode *sema_string_expr(ParserContext *ctx, const Token *t,
                           const char *string, int length) {
    StringNode *p;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(string != NULL || length == 0);
    assert(length >= 0);

    p = malloc(sizeof(*p));
    p->kind = node_string;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->type = array_type_new(type_get_int8(), length + 1);
    p->is_lvalue = false;
    p->string = malloc(sizeof(char) * (length + 1));
    p->len_string = length;

    strncpy(p->string, string, length);
    p->string[length] = '\0';

    return (ExprNode *)p;
}

ExprNode *sema_identifier_expr(ParserContext *ctx, const Token *t) {
    IdentifierNode *p;

    assert(ctx != NULL);
    assert(t != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_identifier;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->type = NULL;
    p->is_lvalue = false;
    p->identifier = str_dup(t->text);
    p->declaration = scope_stack_find(ctx->env, p->identifier, true);

    if (p->declaration == NULL) {
        fprintf(stderr, "error at %s(%d): undeclared symbol %s\n", p->filename,
                p->line, p->identifier);
        exit(1);
    }

    switch (p->declaration->kind) {
    case node_extern:
    case node_variable:
    case node_param:
    case node_function:
        p->type = p->declaration->type;
        p->is_lvalue = true;
        break;

    default:
        fprintf(stderr, "error at %s(%d): symbol %s is not a variable: %d\n",
                p->filename, p->line, p->identifier, p->declaration->kind);
        exit(1);
    }

    return (ExprNode *)p;
}

ExprNode *sema_postfix_expr(ParserContext *ctx, ExprNode *operand,
                            const Token *t) {
    PostfixNode *p;

    assert(ctx != NULL);
    assert(operand != NULL);
    assert(t != NULL);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_postfix;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->type = NULL;
    p->is_lvalue = false;
    p->operand = operand;
    p->operator_ = t->kind;

    switch (p->operator_) {
    case token_increment:
    case token_decrement:
        if (!operand->is_lvalue) {
            fprintf(stderr,
                    "error at %s(%d): "
                    "operand of postfix operator %s must be a lvalue\n",
                    p->operand->filename, p->operand->line, t->text);
            exit(1);
        }

        if (is_incomplete_pointer_type(p->operand->type)) {
            fprintf(stderr,
                    "error at %s(%d): "
                    "operand of postfix operator %s cannot be a pointer "
                    "of incomplete type\n",
                    p->operand->filename, p->operand->line, t->text);
            exit(1);
        }

        if (!is_scalar_type(p->operand->type)) {
            fprintf(stderr,
                    "error at %s(%d): "
                    "invalid operand type of postfix operator %s\n",
                    p->operand->filename, p->operand->line, t->text);
            exit(1);
        }

        p->type = p->operand->type;
        p->is_lvalue = false;
        break;

    default:
        fprintf(stderr, "error at %s(%d): unknown postfix operator %s\n",
                t->filename, t->line, t->text);
        exit(1);
    }

    return (ExprNode *)p;
}

ExprNode *sema_call_expr(ParserContext *ctx, ExprNode *callee,
                         const Token *open, ExprNode **args, int num_args,
                         const Token *close) {
    CallNode *p;
    Type *func_type;
    int num_params;
    bool is_var_args;
    int i;

    assert(ctx != NULL);
    assert(callee != NULL);
    assert(open != NULL);
    assert(args != NULL || num_args == 0);
    assert(num_args >= 0);
    assert(close != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_call;
    p->filename = str_dup(open->filename);
    p->line = open->line;
    p->type = NULL;
    p->is_lvalue = false;
    p->callee = decay_type_conversion(callee);
    p->args = malloc(sizeof(ExprNode *) * num_args);
    p->num_args = num_args;

    for (i = 0; i < num_args; i++) {
        p->args[i] = args[i];
    }

    /* callee type */
    if (!is_function_pointer_type(p->callee->type)) {
        fprintf(stderr, "error at %s(%d): invalid callee type\n",
                p->callee->filename, p->callee->line);
        exit(1);
    }

    func_type = pointer_element_type(p->callee->type);
    num_params = function_count_param_types(func_type);
    is_var_args = function_type_is_var_args(func_type);

    /* check argument types */
    if (num_args != num_params && !(is_var_args && num_args >= num_params)) {
        fprintf(stderr, "error at %s(%d): invalid number of arguments\n",
                p->filename, p->line);
        exit(1);
    }

    for (i = 0; i < num_params; i++) {
        if (!assign_type_conversion(&p->args[i],
                                    function_param_type(func_type, i))) {
            fprintf(stderr, "error at %s(%d): invalid type of argument\n",
                    p->args[i]->filename, p->args[i]->line);
            exit(1);
        }
    }

    /* variadic arguments part */
    for (i = num_params; i < num_args; i++) {
        if (!default_argument_promotion(&p->args[i])) {
            fprintf(stderr,
                    "error at %s(%d): invalid type of varidic argument\n",
                    p->args[i]->filename, p->args[i]->line);
            exit(1);
        }
    }

    /* result type */
    p->type = function_return_type(func_type);

    return (ExprNode *)p;
}

ExprNode *sema_dot_expr(ParserContext *ctx, ExprNode *parent, const Token *t,
                        const Token *identifier) {
    DotNode *p;
    MemberNode *member;

    assert(ctx != NULL);
    assert(parent != NULL);
    assert(t != NULL);
    assert(identifier != NULL);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_dot;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->type = NULL;
    p->is_lvalue = false;
    p->parent = parent;
    p->identifier = str_dup(identifier->text);
    p->index = -1;

    /* check the parent type */
    if (!is_struct_type(parent->type)) {
        fprintf(stderr,
                "error at %s(%d): "
                "member reference base type must be a struct type\n",
                p->parent->filename, p->parent->line);
        exit(1);
    }

    /* resolve member */
    member = struct_type_find_member(parent->type, p->identifier, &p->index);

    if (member == NULL) {
        fprintf(stderr, "error at %s(%d): cannot find member named %s\n",
                identifier->filename, identifier->line, p->identifier);
        exit(1);
    }

    /* resolve expression type */
    p->type = member->type;
    p->is_lvalue = parent->is_lvalue;

    return (ExprNode *)p;
}

ExprNode *sema_arrow_expr(ParserContext *ctx, ExprNode *parent, const Token *t,
                          const Token *identifier) {
    UnaryNode *p;

    assert(ctx != NULL);
    assert(parent != NULL);
    assert(t != NULL);
    assert(identifier != NULL);

    /* T[] -> T* */
    parent = decay_type_conversion(parent);

    /* type check */
    if (!is_pointer_type(parent->type)) {
        fprintf(stderr,
                "error at %s(%d): invalid operand type of opeartor ->\n",
                parent->filename, parent->line);
        exit(1);
    }

    if (is_incomplete_pointer_type(parent->type)) {
        fprintf(stderr,
                "error at %s(%d): "
                "cannot access member of incomplete pointer type\n",
                parent->filename, parent->line);
        exit(1);
    }

    /* make *parent node */
    p = malloc(sizeof(*p));
    p->kind = node_unary;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->type = pointer_element_type(parent->type);
    p->is_lvalue = true;
    p->operator_ = '*';
    p->operand = parent;

    /* make (*parent).member node */
    return sema_dot_expr(ctx, (ExprNode *)p, t, identifier);
}

ExprNode *sema_unary_expr(ParserContext *ctx, const Token *t,
                          ExprNode *operand) {
    UnaryNode *p;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(operand != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_unary;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->type = NULL;
    p->is_lvalue = false;
    p->operator_ = t->kind;
    p->operand = operand;

    switch (t->kind) {
    case '+':
    case '-':
        p->operand = integer_promotion(p->operand);

        if (!is_int32_type(p->operand->type)) {
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of unary operator %s\n",
                p->operand->filename, p->operand->line, t->text);
            exit(1);
        }

        p->type = p->operand->type;
        break;

    case '*':
        p->operand = decay_type_conversion(p->operand);

        if (!is_pointer_type(p->operand->type)) {
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of unary operator %s\n",
                p->operand->filename, p->operand->line, t->text);
            exit(1);
        }

        if (is_incomplete_pointer_type(p->operand->type)) {
            fprintf(stderr,
                    "error at %s(%d): "
                    "cannot dereference pointer of incomplete type\n",
                    p->operand->filename, p->operand->line);
            exit(1);
        }

        p->type = pointer_element_type(p->operand->type);
        p->is_lvalue = true;
        break;

    case '&':
        if (!p->operand->is_lvalue) {
            fprintf(stderr,
                    "error at %s(%d): "
                    "operand of unary operator %s must be a lvalue\n",
                    p->operand->filename, p->operand->line, t->text);
            exit(1);
        }

        p->type = pointer_type_new(p->operand->type);
        break;

    case '!':
        p->operand = integer_promotion(p->operand);

        if (!is_scalar_type(p->operand->type)) {
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of unary operator %s\n",
                p->operand->filename, p->operand->line, t->text);
            exit(1);
        }

        p->type = type_get_int32();
        break;

    case token_increment:
    case token_decrement:
        if (!operand->is_lvalue) {
            fprintf(stderr,
                    "error at %s(%d): "
                    "operand of prefix operator %s must be a lvalue\n",
                    p->operand->filename, p->operand->line, t->text);
            exit(1);
        }

        if (is_incomplete_pointer_type(p->operand->type)) {
            fprintf(stderr,
                    "error at %s(%d): "
                    "operand of prefix operator %s cannot be "
                    "a pointer of incomplete type\n",
                    p->operand->filename, p->operand->line, t->text);
            exit(1);
        }

        if (!is_scalar_type(p->operand->type)) {
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of prefix operator %s\n",
                p->operand->filename, p->operand->line, t->text);
            exit(1);
        }

        p->type = p->operand->type;
        p->is_lvalue = false; /* ++x is rvalue in C */
        break;

    default:
        fprintf(stderr, "error at %s(%d): unknown unary operator %s\n",
                t->filename, t->line, t->text);
        exit(1);
    }

    return (ExprNode *)p;
}

ExprNode *sema_sizeof_expr(ParserContext *ctx, const Token *t, Type *operand) {
    SizeofNode *p;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(operand != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_sizeof;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->type = type_get_int32(); /* TODO: size_t */
    p->is_lvalue = false;
    p->operand = operand;

    /* type check */
    if (is_incomplete_type(p->operand)) {
        fprintf(stderr, "error at %s(%d): cannot get size of incomplete type\n",
                t->filename, t->line);
        exit(1);
    }

    return (ExprNode *)p;
}

ExprNode *sema_cast_expr(ParserContext *ctx, const Token *open, Type *type,
                         const Token *close, ExprNode *operand) {
    CastNode *p;

    assert(ctx != NULL);
    assert(open != NULL);
    assert(type != NULL);
    assert(close != NULL);
    assert(operand != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_cast;
    p->filename = str_dup(open->filename);
    p->line = open->line;
    p->type = type;
    p->is_lvalue = false;
    p->operand = operand;

    if (type_equals(p->type, p->operand->type)) {
        /* T -> T */
        return (ExprNode *)p;
    }

    /* type check */
    if (!can_cast_into(p->operand->type, p->type)) {
        fprintf(stderr, "error at %s(%d): invalid type cast\n", p->filename,
                p->line);
        exit(1);
    }

    return (ExprNode *)p;
}

ExprNode *sema_binary_expr(ParserContext *ctx, ExprNode *left, const Token *t,
                           ExprNode *right) {
    BinaryNode *p;

    assert(ctx != NULL);
    assert(left != NULL);
    assert(t != NULL);
    assert(right != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_binary;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->type = NULL;
    p->is_lvalue = false;
    p->operator_ = t->kind;
    p->left = left;
    p->right = right;

    switch (t->kind) {
    case '+':
        /* a + b */
        usual_arithmetic_conversion(&p->left, &p->right);

        if (is_incomplete_pointer_type(p->left->type) ||
            is_incomplete_pointer_type(p->right->type)) {
            /* <?>* + T or T + <?> */
            fprintf(stderr,
                    "error at %s(%d): "
                    "arithmetic on a pointer to an incomplete type\n",
                    p->filename, p->line);
            exit(1);
        } else if (is_integer_type(p->left->type) &&
                   is_integer_type(p->right->type)) {
            /* int + int -> int */
            p->type = p->left->type;
        } else if (is_pointer_type(p->left->type) &&
                   is_integer_type(p->right->type)) {
            /* T* + int -> T* */
            p->type = p->left->type;
        } else if (is_integer_type(p->left->type) &&
                   is_pointer_type(p->right->type)) {
            /* int + T* -> T* */
            p->type = p->right->type;
        } else {
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of binary operator %s\n",
                p->filename, p->line, t->text);
            exit(1);
        }
        break;

    case '-':
        /* a - b */
        usual_arithmetic_conversion(&p->left, &p->right);

        if (is_integer_type(p->left->type) && is_integer_type(p->right->type)) {
            /* int - int -> int */
            p->type = p->left->type;
        } else if (is_pointer_type(p->left->type) &&
                   is_integer_type(p->right->type)) {
            /* T* - int -> T* */
            p->type = p->left->type;
        } else if (is_pointer_type(p->left->type) &&
                   is_pointer_type(p->right->type)) {
            /* T* - T* -> ptrdiff_t */
            p->type = type_get_int32();
        } else {
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of binary operator %s\n",
                p->filename, p->line, t->text);
            exit(1);
        }
        break;

    case '*':
    case '/':
    case '%':
        /* other arithmetic operator */
        usual_arithmetic_conversion(&p->left, &p->right);

        if (is_integer_type(p->left->type) && is_integer_type(p->right->type)) {
            /* int * int -> int */
            p->type = p->left->type;
        } else {
            /* ? * ? */
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of binary operator %s\n",
                p->filename, p->line, t->text);
            exit(1);
        }
        break;

    case '<':
    case '>':
    case token_lesser_equal:
    case token_greater_equal:
    case token_equal:
    case token_not_equal:
        /* relational operator */
        if (!relational_operation_type_conversion(&p->left, &p->right)) {
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of binary operator %s\n",
                p->filename, p->line, t->text);
            exit(1);
        }

        p->type = type_get_int32();
        break;

    case '&':
    case '^':
    case '|':
        /* bitwise operator */
        usual_arithmetic_conversion(&p->left, &p->right);

        if (!is_int32_type(p->left->type) || !is_int32_type(p->right->type)) {
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of binary operator %s\n",
                p->filename, p->line, t->text);
            exit(1);
        }

        p->type = p->left->type;
        break;

    case token_and:
    case token_or:
        /* logical operator */
        usual_arithmetic_conversion(&p->left, &p->right);

        if (!is_scalar_type(p->left->type) || !is_scalar_type(p->right->type)) {
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of binary operator %s\n",
                p->filename, p->line, t->text);
            exit(1);
        }

        p->type = type_get_int32();
        break;

    case '=':
        /* assignment operator */
        if (!p->left->is_lvalue) {
            fprintf(stderr, "error at %s(%d): cannot assign to rvalue\n",
                    p->left->filename, p->left->line);
            exit(1);
        }

        /* assignment type conversion */
        if (!assign_type_conversion(&p->right, p->left->type)) {
            fprintf(
                stderr,
                "error at %s(%d): invalid operand type of binary operator %s\n",
                p->filename, p->line, t->text);
            exit(1);
        }

        p->type = p->right->type;
        break;

    case '[':
        /* index operator */
        usual_arithmetic_conversion(&p->left, &p->right);

        if (is_pointer_type(p->left->type) && is_integer_type(p->right->type)) {
            p->type = pointer_element_type(p->left->type);
            p->is_lvalue = true;
        } else {
            fprintf(stderr,
                    "error at %s(%d): invalid operand type of operator []\n",
                    p->filename, p->line);
            exit(1);
        }
        break;

    default:
        fprintf(stderr, "error at %s(%d): unknown binary operator %s\n",
                t->filename, t->line, t->text);
        exit(1);
    }

    return (ExprNode *)p;
}

void sema_compound_stmt_enter(ParserContext *ctx) {
    assert(ctx != NULL);

    /* enter scope */
    sema_push_scope(ctx);
}

StmtNode *sema_compound_stmt_leave(ParserContext *ctx, const Token *open,
                                   StmtNode **stmts, int num_stmts,
                                   const Token *close) {
    CompoundNode *p;
    int i;

    assert(ctx != NULL);
    assert(open != NULL);
    assert(stmts != NULL || num_stmts == 0);
    assert(num_stmts >= 0);
    assert(close != NULL);

    /* leave scope */
    sema_pop_scope(ctx);

    p = malloc(sizeof(*p));
    p->kind = node_compound;
    p->filename = str_dup(open->filename);
    p->line = open->line;
    p->stmts = malloc(sizeof(StmtNode *) * num_stmts);
    p->num_stmts = num_stmts;

    for (i = 0; i < num_stmts; i++) {
        p->stmts[i] = stmts[i];
    }

    return (StmtNode *)p;
}

StmtNode *sema_return_stmt(ParserContext *ctx, const Token *t,
                           ExprNode *return_value) {
    ReturnNode *p;
    Type *return_type;

    assert(ctx != NULL);
    assert(ctx->current_function != NULL);
    assert(t != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_return;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->return_value = return_value;

    /* type check */
    return_type = function_return_type(ctx->current_function->type);

    if (is_void_type(return_type) && p->return_value != NULL) {
        fprintf(stderr,
                "error at %s(%d): void function %s should not return a value\n",
                p->filename, p->line, ctx->current_function->identifier);
        exit(1);
    }

    if (!is_void_type(return_type) && p->return_value == NULL) {
        fprintf(stderr,
                "error at %s(%d): non-void function %s should return a value\n",
                p->filename, p->line, ctx->current_function->identifier);
        exit(1);
    }

    if (p->return_value != NULL &&
        !assign_type_conversion(&p->return_value, return_type)) {
        fprintf(stderr, "error at %s(%d): invalid return type\n",
                p->return_value->filename, p->return_value->line);
        exit(1);
    }

    return (StmtNode *)p;
}

void sema_if_stmt_enter_block(ParserContext *ctx) {
    assert(ctx != NULL);

    /* enter scope */
    sema_push_scope(ctx);
}

void sema_if_stmt_leave_block(ParserContext *ctx) {
    assert(ctx != NULL);

    /* leave scope */
    sema_pop_scope(ctx);
}

StmtNode *sema_if_stmt(ParserContext *ctx, const Token *t, ExprNode *condition,
                       StmtNode *then, StmtNode *else_) {
    IfNode *p;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(condition != NULL);
    assert(then != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_if;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->condition = condition;
    p->then = then;
    p->else_ = else_;

    /* type check */
    p->condition = integer_promotion(p->condition);

    if (!is_scalar_type(p->condition->type)) {
        fprintf(stderr, "error at %s(%d): invalid condition type\n",
                p->condition->filename, p->condition->line);
        exit(1);
    }

    return (StmtNode *)p;
}

StmtNode *sema_switch_stmt_case(ParserContext *ctx, const Token *t,
                                ExprNode *case_value, StmtNode **stmts,
                                int num_stmts) {
    CompoundNode *p;
    int i;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(case_value != NULL);
    assert(stmts != NULL || num_stmts == 0);
    assert(num_stmts >= 0);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_compound;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->stmts = malloc(sizeof(StmtNode *) * num_stmts);
    p->num_stmts = num_stmts;

    for (i = 0; i < num_stmts; i++) {
        p->stmts[i] = stmts[i];
    }

    return (StmtNode *)p;
}

StmtNode *sema_switch_stmt_default(ParserContext *ctx, const Token *t,
                                   StmtNode **stmts, int num_stmts) {
    CompoundNode *p;
    int i;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(stmts != NULL || num_stmts == 0);
    assert(num_stmts >= 0);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_compound;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->stmts = malloc(sizeof(StmtNode *) * num_stmts);
    p->num_stmts = num_stmts;

    for (i = 0; i < num_stmts; i++) {
        p->stmts[i] = stmts[i];
    }

    return (StmtNode *)p;
}

void sema_switch_stmt_enter(ParserContext *ctx) {
    /* enter switch scope */
    sema_push_scope(ctx);

    /* accept break */
    control_flow_push_state(ctx, control_flow_state_break_bit);
}

StmtNode *sema_switch_stmt_leave(ParserContext *ctx, const Token *t,
                                 ExprNode *condition, ExprNode **case_values,
                                 StmtNode **cases, int num_cases,
                                 StmtNode *default_) {
    SwitchNode *p;
    int i;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(condition != NULL);
    assert(case_values != NULL || num_cases == 0);
    assert(cases != NULL || num_cases == 0);
    assert(num_cases >= 0);

    /* pop control flow state */
    control_flow_pop_state(ctx);

    /* leave switch scope */
    sema_pop_scope(ctx);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_switch;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->condition = condition;
    p->case_values = malloc(sizeof(ExprNode *) * num_cases);
    p->cases = malloc(sizeof(StmtNode *) * num_cases);
    p->num_cases = num_cases;
    p->default_ = default_;

    for (i = 0; i < num_cases; i++) {
        p->case_values[i] = case_values[i];
        p->cases[i] = cases[i];
    }

    /* type check */
    p->condition = integer_promotion(p->condition);

    if (!is_scalar_type(p->condition->type)) {
        fprintf(stderr, "error at %s(%d): invalid type of switch condition\n",
                p->condition->filename, p->condition->line);
        exit(1);
    }

    for (i = 0; i < num_cases; i++) {
        if (!assign_type_conversion(&p->case_values[i], p->condition->type)) {
            fprintf(stderr, "error %s(%d): invalid type of case condition\n",
                    p->case_values[i]->filename, p->case_values[i]->line);
            exit(1);
        }
    }

    return (StmtNode *)p;
}

void sema_while_stmt_enter_body(ParserContext *ctx) {
    assert(ctx != NULL);

    /* enter scope */
    sema_push_scope(ctx);

    /* push loop state */
    control_flow_push_state(ctx, control_flow_state_break_bit |
                                     control_flow_state_continue_bit);
}

StmtNode *sema_while_stmt_leave_body(ParserContext *ctx, const Token *t,
                                     ExprNode *condition, StmtNode *body) {
    WhileNode *p;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(condition != NULL);
    assert(body != NULL);

    /* pop loop state */
    control_flow_pop_state(ctx);

    /* leave scope */
    sema_pop_scope(ctx);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_while;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->condition = condition;
    p->body = body;

    /* type check */
    p->condition = integer_promotion(p->condition);

    if (!is_scalar_type(p->condition->type)) {
        fprintf(stderr, "error at %s(%d): invalid condition type\n",
                p->condition->filename, p->condition->line);
        exit(1);
    }

    return (StmtNode *)p;
}

void sema_do_stmt_enter_body(ParserContext *ctx) {
    assert(ctx != NULL);

    /* enter scope */
    sema_push_scope(ctx);

    /* push loop state */
    control_flow_push_state(ctx, control_flow_state_break_bit |
                                     control_flow_state_continue_bit);
}

void sema_do_stmt_leave_body(ParserContext *ctx) {
    assert(ctx != NULL);

    /* pop loop state */
    control_flow_pop_state(ctx);

    /* leave scope */
    sema_pop_scope(ctx);
}

StmtNode *sema_do_stmt(ParserContext *ctx, const Token *t, StmtNode *body,
                       ExprNode *condition) {
    DoNode *p;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(body != NULL);
    assert(condition != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_do;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->body = body;
    p->condition = condition;

    /* type check */
    p->condition = integer_promotion(p->condition);

    if (!is_scalar_type(p->condition->type)) {
        fprintf(stderr, "error at %s(%d): invalid condition type\n",
                p->condition->filename, p->condition->line);
        exit(1);
    }

    return (StmtNode *)p;
}

void sema_for_stmt_enter_body(ParserContext *ctx) {
    assert(ctx != NULL);

    /* enter scope */
    sema_push_scope(ctx);

    /* push loop state */
    control_flow_push_state(ctx, control_flow_state_break_bit |
                                     control_flow_state_continue_bit);
}

StmtNode *sema_for_stmt_leave_body(ParserContext *ctx, const Token *t,
                                   ExprNode *initialization,
                                   ExprNode *condition, ExprNode *continuation,
                                   StmtNode *body) {
    ForNode *p;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(body != NULL);

    /* pop loop state */
    control_flow_pop_state(ctx);

    /* leave scope */
    sema_pop_scope(ctx);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_for;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->initialization = initialization;
    p->condition = condition;
    p->continuation = continuation;
    p->body = body;

    /* type check */
    if (p->condition) {
        p->condition = integer_promotion(p->condition);

        if (!is_scalar_type(p->condition->type)) {
            fprintf(stderr, "error at %s(%d): invalid condition type\n",
                    p->condition->filename, p->condition->line);
            exit(1);
        }
    }

    return (StmtNode *)p;
}

StmtNode *sema_break_stmt(ParserContext *ctx, const Token *t) {
    BreakNode *p;

    assert(ctx != NULL);
    assert(t != NULL);

    /* loop check */
    if (!is_break_accepted(ctx)) {
        fprintf(stderr, "error at %s(%d): break outside of loop\n", t->filename,
                t->line);
        exit(1);
    }

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_break;
    p->filename = str_dup(t->filename);
    p->line = t->line;

    return (StmtNode *)p;
}

StmtNode *sema_continue_stmt(ParserContext *ctx, const Token *t) {
    ContinueNode *p;

    assert(ctx != NULL);
    assert(t != NULL);

    /* loop check */
    if (!is_continue_accepted(ctx)) {
        fprintf(stderr, "error at %s(%d): continue outside of loop\n",
                t->filename, t->line);
        exit(1);
    }

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_continue;
    p->filename = str_dup(t->filename);
    p->line = t->line;

    return (StmtNode *)p;
}

StmtNode *sema_decl_stmt(ParserContext *ctx, DeclNode *decl, const Token *t) {
    DeclStmtNode *p;

    assert(ctx != NULL);
    assert(t != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_decl;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->decl = decl;

    return (StmtNode *)p;
}

StmtNode *sema_expr_stmt(ParserContext *ctx, ExprNode *expr, const Token *t) {
    ExprStmtNode *p;

    assert(ctx != NULL);
    assert(expr != NULL);
    assert(t != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_expr;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->expr = expr;

    return (StmtNode *)p;
}

Type *sema_array_declarator(ParserContext *ctx, Type *type, ExprNode *size) {
    int array_size;

    assert(ctx != NULL);
    assert(type != NULL);
    assert(size != NULL);

    /* TODO: more complex constant expression */
    assert(size->kind == node_integer);
    array_size = ((IntegerNode *)size)->value;

    /* size check */
    if (array_size <= 0) {
        fprintf(stderr, "error at %s(%d): invalid array size %d\n",
                size->filename, size->line, array_size);
        exit(1);
    }

    return array_type_new(type, array_size);
}

DeclNode *sema_typedef(ParserContext *ctx, const Token *t, Type *type,
                       const Token *identifier) {
    TypedefNode *p;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(type != NULL);
    assert(identifier != NULL);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_typedef;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->identifier = str_dup(identifier->text);
    p->type = type;
    p->generated_location = NULL;

    /* redefinition check */
    if (scope_stack_find(ctx->env, p->identifier, false)) {
        fprintf(stderr, "error at %s(%d): redefinition of symbol %s\n",
                t->filename, t->line, p->identifier);
        exit(1);
    }

    /* register type symbol */
    scope_stack_register(ctx->env, p->identifier, p);

    return (DeclNode *)p;
}

DeclNode *sema_extern(ParserContext *ctx, const Token *t, Type *type,
                      const Token *identifier) {
    ExternNode *p;
    DeclNode *decl;

    assert(ctx != NULL);
    assert(t != NULL);
    assert(type != NULL);
    assert(identifier != NULL);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_extern;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->identifier = str_dup(identifier->text);
    p->type = type;
    p->generated_location = NULL;

    /* redeclaration check */
    decl = scope_stack_find(ctx->env, p->identifier, false);

    if (decl != NULL) {
        if (decl->kind != node_extern && decl->kind != node_variable) {
            fprintf(stderr, "error at %s(%d): redeclaration of symbol %s\n",
                    p->filename, p->line, p->identifier);
            exit(1);
        }

        if (!type_equals(decl->type, p->type)) {
            fprintf(stderr, "error at %s(%d): conflicting type for %s\n",
                    p->filename, p->line, p->identifier);
            exit(1);
        }
    }

    /* register symbol */
    scope_stack_register(ctx->env, p->identifier, p);

    return (DeclNode *)p;
}

DeclNode *sema_var_decl(ParserContext *ctx, Type *type, const Token *t) {
    VariableNode *p;
    DeclNode *decl;

    assert(ctx != NULL);
    assert(type != NULL);
    assert(t != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_variable;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->identifier = str_dup(t->text);
    p->type = type;
    p->generated_location = NULL;

    /* type check */
    if (is_incomplete_type(type)) {
        fprintf(stderr, "error at %s(%d): variable must have a complete type\n",
                t->filename, t->line);
        exit(1);
    }

    /* redeclaration check */
    decl = scope_stack_find(ctx->env, p->identifier, false);

    if (decl != NULL) {
        if (ctx->current_function != NULL || decl->kind != node_extern) {
            fprintf(stderr,
                    "error at %s(%d): "
                    "symbol %s has already been declared in this scope\n",
                    t->filename, t->line, p->identifier);
            exit(1);
        }
    }

    /* register symbol */
    scope_stack_register(ctx->env, p->identifier, p);

    if (ctx->current_function) {
        /* local variable */
        vec_push(ctx->locals, (DeclNode *)p);
    } else {
        /* global variable */
    }

    return (DeclNode *)p;
}

ParamNode *sema_param(ParserContext *ctx, Type *type, const Token *t) {
    ParamNode *p;

    assert(ctx != NULL);
    assert(type != NULL);
    assert(t != NULL);

    p = malloc(sizeof(*p));
    p->kind = node_param;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->identifier = str_dup(t->text);
    p->type = type;
    p->generated_location = NULL;

    /* type check */
    if (is_incomplete_type(type)) {
        fprintf(stderr,
                "error at %s(%d): parameter must have a complete type\n",
                t->filename, t->line);
        exit(1);
    }

    /* redeclaration check */
    if (scope_stack_find(ctx->env, p->identifier, false)) {
        fprintf(stderr,
                "error at %s(%d): "
                "symbol %s has already been declared in this scope\n",
                t->filename, t->line, p->identifier);
        exit(1);
    }

    /* register symbol */
    scope_stack_register(ctx->env, p->identifier, p);

    return p;
}

void sema_function_enter_params(ParserContext *ctx) {
    assert(ctx != NULL);

    /* enter parameter scope */
    sema_push_scope(ctx);
}

FunctionNode *sema_function_leave_params(ParserContext *ctx, Type *return_type,
                                         const Token *t, ParamNode **params,
                                         int num_params, bool var_args) {
    Type **param_types;
    Type *func_type;
    DeclNode *symbol;
    FunctionNode *p;
    int i;

    assert(ctx != NULL);
    assert(return_type != NULL);
    assert(t != NULL);
    assert(params != NULL || num_params == 0);
    assert(num_params >= 0);

    /* leave parameter scope */
    sema_pop_scope(ctx);

    /* make function type */
    param_types = malloc(sizeof(Type *) * num_params);

    for (i = 0; i < num_params; i++) {
        param_types[i] = params[i]->type;
    }

    func_type =
        function_type_new(return_type, param_types, num_params, var_args);

    /* redeclaration check */
    symbol = scope_stack_find(ctx->env, t->text, false);

    if (symbol != NULL) {
        if (symbol->kind != node_function) {
            fprintf(stderr, "error at %s(%d): redeclaration of symbol %s\n",
                    t->filename, t->line, symbol->identifier);
            exit(1);
        }

        if (!type_equals(symbol->type, func_type)) {
            fprintf(stderr, "error at %s(%d): conflicting type for %s\n",
                    t->filename, t->line, symbol->identifier);
            exit(1);
        }

        if (((FunctionNode *)symbol)->body != NULL) {
            fprintf(stderr, "error at %s(%d): redefinition of function %s\n",
                    t->filename, t->line, symbol->identifier);
            exit(1);
        }
    }

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_function;
    p->filename = str_dup(t->filename);
    p->line = t->line;
    p->identifier = t->text;
    p->type = func_type;
    p->generated_location = NULL;
    p->params = malloc(sizeof(ParamNode *) * num_params);
    p->num_params = num_params;
    p->var_args = var_args;
    p->body = NULL;
    p->locals = NULL;
    p->num_locals = 0;

    for (i = 0; i < num_params; i++) {
        p->params[i] = params[i];
    }

    /* register symbol */
    scope_stack_register(ctx->env, p->identifier, p);

    return p;
}

void sema_function_enter_body(ParserContext *ctx, FunctionNode *p) {
    int i;

    assert(ctx != NULL);
    assert(p != NULL);

    ctx->current_function = p;
    ctx->locals = vec_new();

    /* enter parameter scope */
    sema_push_scope(ctx);

    /* register parameters */
    for (i = 0; i < p->num_params; i++) {
        scope_stack_register(ctx->env, p->params[i]->identifier, p->params[i]);
    }
}

FunctionNode *sema_function_leave_body(ParserContext *ctx, FunctionNode *p,
                                       StmtNode *body) {
    int i;

    assert(ctx != NULL);
    assert(ctx->locals != NULL);
    assert(p != NULL);
    assert(body != NULL);

    p->body = body;

    p->locals = malloc(sizeof(DeclNode *) * ctx->locals->size);
    p->num_locals = ctx->locals->size;

    for (i = 0; i < p->num_locals; i++) {
        p->locals[i] = ctx->locals->data[i];
    }

    /* leave parameter scope */
    sema_pop_scope(ctx);

    ctx->current_function = NULL;
    ctx->locals = NULL;

    return p;
}

ParserContext *sema_translation_unit_enter(const Token **tokens) {
    ParserContext *ctx;

    assert(tokens != NULL);
    assert(*tokens != NULL);

    ctx = malloc(sizeof(*ctx));
    ctx->env = scope_stack_new();
    ctx->struct_env = scope_stack_new();
    ctx->tokens = tokens;
    ctx->index = 0;
    ctx->current_function = NULL;
    ctx->locals = NULL;
    ctx->flow_state = control_flow_state_new();

    return ctx;
}

TranslationUnitNode *sema_translation_unit_leave(ParserContext *ctx,
                                                 const char *filename,
                                                 DeclNode **decls,
                                                 int num_decls) {
    TranslationUnitNode *p;
    int i;

    assert(ctx != NULL);
    assert(scope_stack_depth(ctx->env) == 1);
    assert(scope_stack_depth(ctx->struct_env) == 1);
    assert(ctx->flow_state->size == 1);
    assert(filename != NULL);
    assert(decls != NULL || num_decls == 0);
    assert(num_decls >= 0);

    p = malloc(sizeof(*p));
    p->filename = str_dup(filename);
    p->decls = malloc(sizeof(DeclNode *) * num_decls);
    p->num_decls = num_decls;

    for (i = 0; i < num_decls; i++) {
        p->decls[i] = decls[i];
    }

    return p;
}
