#include "nocc.h"

enum {
    control_flow_state_none = 0,
    control_flow_state_break_bit = 1,
    control_flow_state_continue_bit = 2,
};

Vec *control_flow_state_new(void) {
    Vec *state;

    state = vec_new();
    vec_push(state, (void *)(intptr_t)control_flow_state_none);

    return state;
}

int control_flow_current_state(ParserContext *ctx) {
    assert(ctx);
    assert(ctx->flow_state->size > 0);

    return (intptr_t)vec_back(ctx->flow_state);
}

void control_flow_push_state(ParserContext *ctx, int flow_state) {
    assert(ctx);

    vec_push(ctx->flow_state,
             (void *)((intptr_t)flow_state | control_flow_current_state(ctx)));
}

void control_flow_pop_state(ParserContext *ctx) {
    assert(ctx);
    assert(ctx->flow_state->size > 1);

    vec_pop(ctx->flow_state);
}

bool is_break_accepted(ParserContext *ctx) {
    assert(ctx);
    return control_flow_current_state(ctx) & control_flow_state_break_bit;
}

bool is_continue_accepted(ParserContext *ctx) {
    assert(ctx);
    return control_flow_current_state(ctx) & control_flow_state_continue_bit;
}

ExprNode *sema_paren_expr(ParserContext *ctx, const Token *open, ExprNode *expr,
                          const Token *close) {
    assert(ctx);
    assert(open);
    assert(expr);
    assert(close);

    return expr;
}

ExprNode *sema_integer_expr(ParserContext *ctx, const Token *t, int value) {
    IntegerNode *p;

    assert(ctx);
    assert(t);

    p = malloc(sizeof(*p));
    p->kind = node_integer;
    p->line = t->line;
    p->type = type_get_int32();
    p->is_lvalue = false;
    p->value = value;

    return (ExprNode *)p;
}

ExprNode *sema_identifier_expr(ParserContext *ctx, const Token *t) {
    IdentifierNode *p;

    assert(ctx);
    assert(t);

    p = malloc(sizeof(*p));
    p->kind = node_identifier;
    p->line = t->line;
    p->type = NULL;
    p->is_lvalue = false;
    p->identifier = strdup(t->text);
    p->declaration = scope_stack_find(ctx->env, p->identifier, true);

    if (p->declaration == NULL) {
        fprintf(stderr, "undeclared symbol %s\n", p->identifier);
        exit(1);
    }

    p->type = p->declaration->type;
    p->is_lvalue = true;

    return (ExprNode *)p;
}

ExprNode *sema_call_expr(ParserContext *ctx, ExprNode *callee,
                         const Token *open, ExprNode **args, int num_args,
                         const Token *close) {
    CallNode *p;
    FunctionType *func_type;
    int i;

    assert(ctx);
    assert(callee);
    assert(open);
    assert(args != NULL || num_args == 0);
    assert(num_args >= 0);
    assert(close);

    p = malloc(sizeof(*p));
    p->kind = node_call;
    p->line = open->line;
    p->type = NULL;
    p->is_lvalue = false;
    p->callee = callee;
    p->args = malloc(sizeof(ExprNode *) * num_args);
    p->num_args = num_args;

    for (i = 0; i < num_args; i++) {
        p->args[i] = args[i];
    }

    /* callee type */
    /* TODO: fix to function pointer */
    if (callee->type->kind != type_function) {
        fprintf(stderr, "invalid callee type\n");
        exit(1);
    }

    func_type = (FunctionType *)callee->type;

    /* check argument types */
    if (num_args != func_type->num_params) {
        fprintf(stderr, "invalid number of arguments\n");
        exit(1);
    }

    for (i = 0; i < num_args; i++) {
        /* TODO: remove type limitation */
        if (args[i]->type != func_type->param_types[i] ||
            args[i]->type != type_get_int32()) {
            fprintf(stderr, "invalid type of argument\n");
            exit(1);
        }
    }

    /* result type */
    p->type = func_type->return_type;

    return (ExprNode *)p;
}

ExprNode *sema_unary_expr(ParserContext *ctx, const Token *t,
                          ExprNode *operand) {
    UnaryNode *p;

    assert(ctx);
    assert(t);
    assert(operand);

    p = malloc(sizeof(*p));
    p->kind = node_unary;
    p->line = t->line;
    p->type = NULL;
    p->is_lvalue = false;
    p->operator_ = t->kind;
    p->operand = operand;

    switch (t->kind) {
    case '+':
    case '-':
        if (operand->type != type_get_int32()) {
            fprintf(stderr, "invalid operand type of unary operator %s\n",
                    t->text);
            exit(1);
        }

        p->type = operand->type;
        break;

    default:
        fprintf(stderr, "unknown unary operator %s\n", t->text);
        exit(1);
    }

    return (ExprNode *)p;
}

ExprNode *sema_binary_expr(ParserContext *ctx, ExprNode *left, const Token *t,
                           ExprNode *right) {
    BinaryNode *p;

    assert(ctx);
    assert(left);
    assert(t);
    assert(right);

    p = malloc(sizeof(*p));
    p->kind = node_binary;
    p->line = t->line;
    p->type = NULL;
    p->is_lvalue = false;
    p->operator_ = t->kind;
    p->left = left;
    p->right = right;

    switch (t->kind) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
        /* arithmetic operator */
        if (left->type != type_get_int32() || right->type != type_get_int32()) {
            fprintf(stderr, "invalid operand type of binary operator %s\n",
                    t->text);
            exit(1);
        }

        p->type = left->type;
        break;

    case '<':
    case '>':
    case token_lesser_equal:
    case token_greater_equal:
    case token_equal:
    case token_not_equal:
        /* relational operator */
        if (left->type != type_get_int32() || right->type != type_get_int32()) {
            fprintf(stderr, "invalid operand type of binary operator %s\n",
                    t->text);
            exit(1);
        }

        p->type = type_get_int32();
        break;

    case '=':
        /* assignment operator */
        if (!left->is_lvalue) {
            fprintf(stderr, "cannot assign to rvalue\n");
            exit(1);
        }

        if (left->type != type_get_int32() || right->type != type_get_int32()) {
            fprintf(stderr, "invalid operand type of binary operator %s\n",
                    t->text);
            exit(1);
        }

        p->type = right->type;
        break;

    default:
        fprintf(stderr, "unknown binary operator %s\n", t->text);
        exit(1);
    }

    return (ExprNode *)p;
}

void sema_compound_stmt_enter(ParserContext *ctx) {
    assert(ctx);

    /* enter scope */
    scope_stack_push(ctx->env);
}

StmtNode *sema_compound_stmt_leave(ParserContext *ctx, const Token *open,
                                   StmtNode **stmts, int num_stmts,
                                   const Token *close) {
    CompoundNode *p;
    int i;

    assert(ctx);
    assert(open);
    assert(stmts != NULL || num_stmts == 0);
    assert(num_stmts >= 0);
    assert(close);

    /* leave scope */
    scope_stack_pop(ctx->env);

    p = malloc(sizeof(*p));
    p->kind = node_compound;
    p->line = open->line;
    p->stmts = malloc(sizeof(StmtNode *) * num_stmts);
    p->num_stmts = num_stmts;

    for (i = 0; i < num_stmts; i++) {
        p->stmts[i] = stmts[i];
    }

    return (StmtNode *)p;
}

StmtNode *sema_return_stmt(ParserContext *ctx, const Token *t,
                           ExprNode *return_value, const Token *semi) {
    ReturnNode *p;
    FunctionType *function_type;

    assert(ctx);
    assert(ctx->current_function);
    assert(t);
    assert(semi);

    p = malloc(sizeof(*p));
    p->kind = node_return;
    p->line = t->line;
    p->return_value = return_value;

    /* type check */
    function_type = (FunctionType *)ctx->current_function->type;

    if (function_type->return_type == type_get_void()) {
        if (return_value != NULL) {
            fprintf(stderr, "void function should not return a value\n");
            exit(1);
        }
    } else {
        if (return_value == NULL ||
            function_type->return_type != return_value->type) {
            fprintf(stderr, "invalid return type\n");
            exit(1);
        }
    }

    return (StmtNode *)p;
}

void sema_if_stmt_enter_block(ParserContext *ctx) {
    assert(ctx);

    /* enter scope */
    scope_stack_push(ctx->env);
}

void sema_if_stmt_leave_block(ParserContext *ctx) {
    assert(ctx);

    /* leave scope */
    scope_stack_pop(ctx->env);
}

StmtNode *sema_if_stmt(ParserContext *ctx, const Token *t, ExprNode *condition,
                       StmtNode *then, StmtNode *else_) {
    IfNode *p;

    assert(ctx);
    assert(t);
    assert(condition);
    assert(then);

    p = malloc(sizeof(*p));
    p->kind = node_if;
    p->line = t->line;
    p->condition = condition;
    p->then = then;
    p->else_ = else_;

    /* type check */
    if (condition->type != type_get_int32()) {
        fprintf(stderr, "invalid condition type\n");
        exit(1);
    }

    return (StmtNode *)p;
}

void sema_while_stmt_enter_body(ParserContext *ctx) {
    assert(ctx);

    /* enter scope */
    scope_stack_push(ctx->env);

    /* push loop state */
    control_flow_push_state(ctx, control_flow_state_break_bit |
                                     control_flow_state_continue_bit);
}

StmtNode *sema_while_stmt_leave_body(ParserContext *ctx, const Token *t,
                                     ExprNode *condition, StmtNode *body) {
    WhileNode *p;

    assert(ctx);
    assert(t);
    assert(condition);
    assert(body);

    /* pop loop state */
    control_flow_pop_state(ctx);

    /* leave scope */
    scope_stack_pop(ctx->env);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_while;
    p->line = t->line;
    p->condition = condition;
    p->body = body;

    /* type check */
    if (condition->type != type_get_int32()) {
        fprintf(stderr, "invalid condition type\n");
        exit(1);
    }

    return (StmtNode *)p;
}

void sema_do_stmt_enter_body(ParserContext *ctx) {
    assert(ctx);

    /* enter scope */
    scope_stack_push(ctx->env);

    /* push loop state */
    control_flow_push_state(ctx, control_flow_state_break_bit |
                                     control_flow_state_continue_bit);
}

void sema_do_stmt_leave_body(ParserContext *ctx) {
    assert(ctx);

    /* pop loop state */
    control_flow_pop_state(ctx);

    /* leave scope */
    scope_stack_pop(ctx->env);
}

StmtNode *sema_do_stmt(ParserContext *ctx, const Token *t, StmtNode *body,
                       ExprNode *condition) {
    DoNode *p;

    assert(ctx);
    assert(t);
    assert(body);
    assert(condition);

    p = malloc(sizeof(*p));
    p->kind = node_do;
    p->line = t->line;
    p->body = body;
    p->condition = condition;

    /* type check */
    if (condition->type != type_get_int32()) {
        fprintf(stderr, "invalid condition type\n");
        exit(1);
    }

    return (StmtNode *)p;
}

void sema_for_stmt_enter_body(ParserContext *ctx) {
    assert(ctx);

    /* enter scope */
    scope_stack_push(ctx->env);

    /* push loop state */
    control_flow_push_state(ctx, control_flow_state_break_bit |
                                     control_flow_state_continue_bit);
}

StmtNode *sema_for_stmt_leave_body(ParserContext *ctx, const Token *t,
                                   ExprNode *initialization,
                                   ExprNode *condition, ExprNode *continuation,
                                   StmtNode *body) {
    ForNode *p;

    assert(ctx);
    assert(t);
    assert(body);

    /* pop loop state */
    control_flow_pop_state(ctx);

    /* leave scope */
    scope_stack_pop(ctx->env);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_for;
    p->line = t->line;
    p->initialization = initialization;
    p->condition = condition;
    p->continuation = continuation;
    p->body = body;

    /* type check */
    if (condition && condition->type != type_get_int32()) {
        fprintf(stderr, "invalid condition type\n");
        exit(1);
    }

    return (StmtNode *)p;
}

StmtNode *sema_break_stmt(ParserContext *ctx, const Token *t) {
    BreakNode *p;

    assert(ctx);
    assert(t);

    /* loop check */
    if (!is_break_accepted(ctx)) {
        fprintf(stderr, "break outside of loop\n");
        exit(1);
    }

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_break;
    p->line = t->line;

    return (StmtNode *)p;
}

StmtNode *sema_continue_stmt(ParserContext *ctx, const Token *t) {
    ContinueNode *p;

    assert(ctx);
    assert(t);

    /* loop check */
    if (!is_continue_accepted(ctx)) {
        fprintf(stderr, "continue outside of loop\n");
        exit(1);
    }

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_continue;
    p->line = t->line;

    return (StmtNode *)p;
}

StmtNode *sema_decl_stmt(ParserContext *ctx, DeclNode *decl, const Token *t) {
    DeclStmtNode *p;

    assert(ctx);
    assert(decl);
    assert(t);

    p = malloc(sizeof(*p));
    p->kind = node_decl;
    p->line = t->line;
    p->decl = decl;

    return (StmtNode *)p;
}

StmtNode *sema_expr_stmt(ParserContext *ctx, ExprNode *expr, const Token *t) {
    ExprStmtNode *p;

    assert(ctx);
    assert(expr);
    assert(t);

    p = malloc(sizeof(*p));
    p->kind = node_expr;
    p->line = t->line;
    p->expr = expr;

    return (StmtNode *)p;
}

DeclNode *sema_var_decl(ParserContext *ctx, Type *type, const Token *t) {
    VariableNode *p;

    assert(ctx);
    assert(type);
    assert(t);

    p = malloc(sizeof(*p));
    p->kind = node_variable;
    p->line = t->line;
    p->identifier = strdup(t->text);
    p->type = type;
    p->generated_location = NULL;

    /* type check */
    if (type == type_get_void()) {
        fprintf(stderr, "variable type must not be void\n");
        exit(1);
    }

    /* redeclaration check */
    if (scope_stack_find(ctx->env, p->identifier, false)) {
        fprintf(stderr, "symbol %s has already been declared in this scope\n",
                p->identifier);
        exit(1);
    }

    /* register symbol */
    scope_stack_register(ctx->env, (DeclNode *)p);

    if (ctx->current_function) {
        /* local variable */
        vec_push(ctx->locals, (DeclNode *)p);
    } else {
        /* TODO: global variable */
        assert(0);
    }

    return (DeclNode *)p;
}

ParamNode *sema_param(ParserContext *ctx, Type *type, const Token *t) {
    ParamNode *p;

    assert(ctx);
    assert(type);
    assert(t);

    p = malloc(sizeof(*p));
    p->kind = node_param;
    p->line = t->line;
    p->identifier = strdup(t->text);
    p->type = type;
    p->generated_location = NULL;

    /* type check */
    if (type == type_get_void()) {
        fprintf(stderr, "parameter type must not be void\n");
        exit(1);
    }

    /* redeclaration check */
    if (scope_stack_find(ctx->env, p->identifier, false)) {
        fprintf(stderr, "symbol %s has already been declared in this scope\n",
                p->identifier);
        exit(1);
    }

    /* register symbol */
    scope_stack_register(ctx->env, (DeclNode *)p);

    return p;
}

void sema_function_enter_params(ParserContext *ctx) {
    assert(ctx);

    /* enter parameter scope */
    scope_stack_push(ctx->env);
}

FunctionNode *sema_function_leave_params(ParserContext *ctx, Type *return_type,
                                         const Token *t, ParamNode **params,
                                         int num_params, bool var_args) {
    Type **param_types;
    Type *function_type;
    FunctionNode *p;
    int i;

    assert(ctx);
    assert(return_type);
    assert(t);
    assert(params != NULL || num_params == 0);
    assert(num_params >= 0);

    /* leave parameter scope */
    scope_stack_pop(ctx->env);

    /* make function type */
    param_types = malloc(sizeof(Type *) * num_params);

    for (i = 0; i < num_params; i++) {
        param_types[i] = params[i]->type;
    }

    function_type = function_type_new(return_type, param_types, num_params);

    /* make node */
    p = malloc(sizeof(*p));
    p->kind = node_function;
    p->line = t->line;
    p->identifier = t->text;
    p->type = function_type;
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

    /* redeclaration check */
    if (scope_stack_find(ctx->env, p->identifier, false)) {
        fprintf(stderr, "symbol %s has already been declared in this scope\n",
                p->identifier);
        exit(1);
    }

    /* register symbol */
    scope_stack_register(ctx->env, (DeclNode *)p);

    return p;
}

void sema_function_enter_body(ParserContext *ctx, FunctionNode *p) {
    int i;

    assert(ctx);
    assert(p);

    ctx->current_function = p;
    ctx->locals = vec_new();

    /* enter parameter scope */
    scope_stack_push(ctx->env);

    /* register parameters */
    for (i = 0; i < p->num_params; i++) {
        scope_stack_register(ctx->env, (DeclNode *)p->params[i]);
    }
}

FunctionNode *sema_function_leave_body(ParserContext *ctx, FunctionNode *p,
                                       StmtNode *body) {
    int i;

    assert(ctx);
    assert(ctx->locals);
    assert(p);
    assert(body);

    p->body = body;

    p->locals = malloc(sizeof(DeclNode *) * ctx->locals->size);
    p->num_locals = ctx->locals->size;

    for (i = 0; i < p->num_locals; i++) {
        p->locals[i] = ctx->locals->data[i];
    }

    /* leave parameter scope */
    scope_stack_pop(ctx->env);

    ctx->current_function = NULL;
    ctx->locals = NULL;

    return p;
}

ParserContext *sema_translation_unit_enter(const char *src) {
    ParserContext *ctx;
    Vec *tokens;

    assert(src);

    tokens = lex(src);

    ctx = malloc(sizeof(*ctx));
    ctx->env = scope_stack_new();
    ctx->tokens = (const Token **)tokens->data;
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

    assert(ctx);
    assert(scope_stack_depth(ctx->env) == 1);
    assert(ctx->flow_state->size == 1);
    assert(filename);
    assert(decls != NULL || num_decls == 0);
    assert(num_decls >= 0);

    p = malloc(sizeof(*p));
    p->filename = strdup(filename);
    p->decls = malloc(sizeof(DeclNode *) * num_decls);
    p->num_decls = num_decls;

    for (i = 0; i < num_decls; i++) {
        p->decls[i] = decls[i];
    }

    return p;
}
