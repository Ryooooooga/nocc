#include "nocc.h"

const Token *current_token(ParserContext *ctx) {
    assert(ctx);
    return ctx->tokens[ctx->index];
}

const Token *peek_token(ParserContext *ctx) {
    assert(ctx);

    if (current_token(ctx)->kind == '\0') {
        return current_token(ctx);
    }

    return ctx->tokens[ctx->index + 1];
}

const Token *consume_token(ParserContext *ctx) {
    assert(ctx);

    if (current_token(ctx)->kind == '\0') {
        return current_token(ctx);
    }

    return ctx->tokens[ctx->index++];
}

bool is_type_specifier_token(ParserContext *ctx, const Token *t) {
    DeclNode *symbol;

    assert(ctx);
    assert(t);

    switch (t->kind) {
    case token_void:
    case token_char:
    case token_int:
    case token_struct:
    case token_const:
        return true;

    case token_identifier:
        symbol = scope_stack_find(ctx->env, t->text, true);
        return symbol->kind == node_typedef;

    default:
        return false;
    }
}

bool is_declaration_specifier_token(ParserContext *ctx, const Token *t) {
    switch (t->kind) {
    case token_typedef:
        return true;

    default:
        return is_type_specifier_token(ctx, t);
    }
}

MemberNode *parse_struct_member(ParserContext *ctx) {
    Type *type;
    const Token *t;

    /* type */
    type = parse_type(ctx);

    /* identifier */
    t = consume_token(ctx);

    if (t->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", t->text);
        exit(1);
    }

    /* ; */
    if (current_token(ctx)->kind != ';') {
        fprintf(stderr, "expected ;, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    /* make node */
    return sema_struct_member(ctx, type, t);
}

Type *parse_identifier_type(ParserContext *ctx) {
    const Token *t;

    /* identifier */
    t = consume_token(ctx);

    if (t->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", t->text);
    }

    /* get type */
    return sema_identifier_type(ctx, t);
}

Type *parse_struct_type(ParserContext *ctx) {
    StructType *type;

    const Token *t;
    const Token *identifier;
    Vec *members;

    /* struct */
    t = consume_token(ctx);

    if (t->kind != token_struct) {
        fprintf(stderr, "expected struct, but got %s\n", t->text);
        exit(1);
    }

    /* identifier */
    identifier = consume_token(ctx);

    if (identifier->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", identifier->text);
        exit(1);
    }

    /* { */
    if (current_token(ctx)->kind != '{') {
        return sema_struct_type_without_body(ctx, t, identifier);
    }
    consume_token(ctx);

    /* register symbol and enter struct scope */
    type = sema_struct_type_enter(ctx, t, identifier);

    /* member declarations */
    members = vec_new();

    while (current_token(ctx)->kind != '}') {
        vec_push(members, parse_struct_member(ctx));
    }

    /* } */
    if (current_token(ctx)->kind != '}') {
        fprintf(stderr, "expected }, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    /* leave struct scope and make node */
    return sema_struct_type_leave(ctx, type, (MemberNode **)members->data,
                                  members->size);
}

Type *parse_primary_type(ParserContext *ctx) {
    switch (current_token(ctx)->kind) {
    case token_void:
        consume_token(ctx); /* eat void */
        return type_get_void();

    case token_char:
        consume_token(ctx); /* eat char */
        return type_get_int8();

    case token_int:
        consume_token(ctx); /* eat int */
        return type_get_int32();

    case token_identifier:
        return parse_identifier_type(ctx);

    case token_struct:
        return parse_struct_type(ctx);

    default:
        fprintf(stderr, "expected type, but got %s\n",
                current_token(ctx)->text);
        exit(1);
    }
}

Type *parse_type(ParserContext *ctx) {
    bool is_const;
    Type *type;

    assert(ctx);

    /* const? */
    is_const = false;

    if (current_token(ctx)->kind == token_const) {
        consume_token(ctx); /* eat const */

        is_const = true;
    }

    /* primary type */
    type = parse_primary_type(ctx);

    /* TODO: const type */
    (void)is_const;

    /* pointer type */
    while (current_token(ctx)->kind == '*') {
        /* * */
        consume_token(ctx);

        type = pointer_type_new(type);
    }

    return type;
}

ExprNode *parse_paren_expr(ParserContext *ctx) {
    const Token *open;
    const Token *close;
    ExprNode *expr;

    /* ( */
    open = consume_token(ctx);

    if (open->kind != '(') {
        fprintf(stderr, "expected (, but got %s\n", open->text);
        exit(1);
    }

    /* expression */
    expr = parse_expr(ctx);

    /* ) */
    close = consume_token(ctx);

    if (close->kind != ')') {
        fprintf(stderr, "expected ), but got %s\n", close->text);
        exit(1);
    }

    /* make node */
    return sema_paren_expr(ctx, open, expr, close);
}

ExprNode *parse_number_expr(ParserContext *ctx) {
    const Token *t;
    long value;

    /* number */
    t = consume_token(ctx);

    if (t->kind != token_number) {
        fprintf(stderr, "expected number, but got %s\n", t->text);
        exit(1);
    }

    /* convert */
    errno = 0;
    value = strtol(t->text, NULL, 10);

    if (errno == ERANGE || value > INT_MAX) {
        fprintf(stderr, "too large integer constant %s\n", t->text);
        exit(1);
    }

    /* make node */
    return sema_integer_expr(ctx, t, (int)value);
}

ExprNode *parse_string_expr(ParserContext *ctx) {
    const Token *t;

    /* string */
    t = consume_token(ctx);

    if (t->kind != token_string) {
        fprintf(stderr, "expected string, but got %s\n", t->text);
        exit(1);
    }

    /* make node */
    return sema_string_expr(ctx, t, t->string, t->len_string);
}

ExprNode *parse_identifier_expr(ParserContext *ctx) {
    const Token *t;

    /* identifier */
    t = consume_token(ctx);

    if (t->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", t->text);
        exit(1);
    }

    /* make node */
    return sema_identifier_expr(ctx, t);
}

ExprNode *parse_primary_expr(ParserContext *ctx) {
    switch (current_token(ctx)->kind) {
    case '(':
        return parse_paren_expr(ctx);

    case token_number:
        return parse_number_expr(ctx);

    case token_string:
        return parse_string_expr(ctx);

    case token_identifier:
        return parse_identifier_expr(ctx);

    default:
        fprintf(stderr, "expected expression, but got %s\n",
                current_token(ctx)->text);
        exit(1);
    }
}

ExprNode *parse_call_expr(ParserContext *ctx, ExprNode *callee) {
    const Token *open;
    const Token *close;
    Vec *args;

    /* ( */
    open = consume_token(ctx);

    if (open->kind != '(') {
        fprintf(stderr, "expected (, but got %s\n", open->text);
        exit(1);
    }

    /* argument list */
    args = vec_new();

    if (current_token(ctx)->kind != ')') {
        /* expression */
        vec_push(args, parse_assign_expr(ctx));

        /* {, expression} */
        while (current_token(ctx)->kind == ',') {
            /* , */
            consume_token(ctx);

            /* expression */
            vec_push(args, parse_assign_expr(ctx));
        }
    }

    /* ) */
    close = consume_token(ctx);

    if (close->kind != ')') {
        fprintf(stderr, "expected ), but got %s\n", close->text);
        exit(1);
    }

    /* make node */
    return sema_call_expr(ctx, callee, open, (ExprNode **)args->data,
                          args->size, close);
}

ExprNode *parse_dot_expr(ParserContext *ctx, ExprNode *parent) {
    const Token *t;
    const Token *identifier;

    /* . */
    t = consume_token(ctx);

    if (t->kind != '.') {
        fprintf(stderr, "expected ., but got %s\n", t->text);
        exit(1);
    }

    /* identifier */
    identifier = consume_token(ctx);

    if (identifier->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", identifier->text);
        exit(1);
    }

    /* make node */
    return sema_dot_expr(ctx, parent, t, identifier);
}

ExprNode *parse_postfix_expr(ParserContext *ctx) {
    ExprNode *operand;

    operand = parse_primary_expr(ctx);

    while (1) {
        switch (current_token(ctx)->kind) {
        case '(':
            operand = parse_call_expr(ctx, operand);
            break;

        case '.':
            operand = parse_dot_expr(ctx, operand);
            break;

        default:
            return operand;
        }
    }
}

ExprNode *parse_cast_expr(ParserContext *ctx) {
    const Token *open;
    const Token *close;
    Type *type;
    ExprNode *operand;

    /* ( */
    open = consume_token(ctx);

    if (open->kind != '(') {
        fprintf(stderr, "expected (, but got %s\n", open->text);
        exit(1);
    }

    /* type */
    type = parse_type(ctx);

    /* ) */
    close = consume_token(ctx);

    if (close->kind != ')') {
        fprintf(stderr, "expected ), but got %s\n", close->text);
        exit(1);
    }

    /* unary expression */
    operand = parse_unary_expr(ctx);

    /* make node */
    return sema_cast_expr(ctx, open, type, close, operand);
}

ExprNode *parse_unary_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *operand;

    assert(ctx);

    t = current_token(ctx);

    switch (t->kind) {
    case '-':
    case '*':
    case '&':
        /* unary operator */
        consume_token(ctx);

        /* unary expression */
        operand = parse_unary_expr(ctx);

        /* make node */
        return sema_unary_expr(ctx, t, operand);

    case '(':
        if (is_type_specifier_token(ctx, peek_token(ctx))) {
            return parse_cast_expr(ctx);
        }
        break;

    default:
        break;
    }

    return parse_postfix_expr(ctx);
}

ExprNode *parse_multiplicative_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* unary expression */
    left = parse_unary_expr(ctx);

    while (current_token(ctx)->kind == '*' || current_token(ctx)->kind == '/' ||
           current_token(ctx)->kind == '%') {
        /* multiplicative operator */
        t = consume_token(ctx);

        /* unary expression */
        right = parse_unary_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_additive_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* multiplicative expression */
    left = parse_multiplicative_expr(ctx);

    while (current_token(ctx)->kind == '+' || current_token(ctx)->kind == '-') {
        /* additive operator */
        t = consume_token(ctx);

        /* multiplicative expression */
        right = parse_multiplicative_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_relational_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* additive expression */
    left = parse_additive_expr(ctx);

    while (current_token(ctx)->kind == '<' || current_token(ctx)->kind == '>' ||
           current_token(ctx)->kind == token_lesser_equal ||
           current_token(ctx)->kind == token_greater_equal) {
        /* relational operator */
        t = consume_token(ctx);

        /* additive expression */
        right = parse_additive_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_equality_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* relational expression */
    left = parse_relational_expr(ctx);

    while (current_token(ctx)->kind == token_equal ||
           current_token(ctx)->kind == token_not_equal) {
        /* equality operator */
        t = consume_token(ctx);

        /* relational expression */
        right = parse_relational_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_assign_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    assert(ctx);

    /* equality expression */
    left = parse_equality_expr(ctx);

    /* assignment operator */
    if (current_token(ctx)->kind != '=') {
        return left;
    }

    t = consume_token(ctx);

    /* assignment expression */
    right = parse_assign_expr(ctx);

    /* make node */
    return sema_binary_expr(ctx, left, t, right);
}

ExprNode *parse_expr(ParserContext *ctx) {
    assert(ctx);

    return parse_assign_expr(ctx);
}

StmtNode *parse_compound_stmt(ParserContext *ctx) {
    const Token *open;
    const Token *close;
    Vec *stmts;

    /* enter scope */
    sema_compound_stmt_enter(ctx);

    /* { */
    open = consume_token(ctx);

    if (open->kind != '{') {
        fprintf(stderr, "expected {, but got %s\n", open->text);
        exit(1);
    }

    /* {statement} */
    stmts = vec_new();

    while (current_token(ctx)->kind != '}') {
        vec_push(stmts, parse_stmt(ctx));
    }

    /* } */
    close = consume_token(ctx);

    if (close->kind != '}') {
        fprintf(stderr, "expected }, but got %s\n", close->text);
        exit(1);
    }

    /* make node and leave scope */
    return sema_compound_stmt_leave(ctx, open, (StmtNode **)stmts->data,
                                    stmts->size, close);
}

StmtNode *parse_return_stmt(ParserContext *ctx) {
    const Token *t;
    const Token *semi;
    ExprNode *return_value;

    /* return */
    t = consume_token(ctx);

    if (t->kind != token_return) {
        fprintf(stderr, "expected return, but got %s\n", t->text);
        exit(1);
    }

    /* expression */
    return_value = NULL;

    if (current_token(ctx)->kind != ';') {
        return_value = parse_expr(ctx);
    }

    /* ; */
    semi = consume_token(ctx);

    if (semi->kind != ';') {
        fprintf(stderr, "expected ;, but got %s\n", semi->text);
        exit(1);
    }

    /* make node */
    return sema_return_stmt(ctx, t, return_value, semi);
}

StmtNode *parse_if_stmt(ParserContext *ctx) {
    const Token *t;
    ExprNode *condition;
    StmtNode *then;
    StmtNode *else_;

    /* if */
    t = consume_token(ctx);

    if (t->kind != token_if) {
        fprintf(stderr, "expected if, but got %s\n", t->text);
        exit(1);
    }

    /* ( expression ) */
    condition = parse_paren_expr(ctx);

    /* enter then scope */
    sema_if_stmt_enter_block(ctx);

    /* statement */
    then = parse_stmt(ctx);

    /* leave then scope */
    sema_if_stmt_leave_block(ctx);

    /* else */
    else_ = NULL;

    if (current_token(ctx)->kind == token_else) {
        consume_token(ctx);

        /* enter else scope */
        sema_if_stmt_enter_block(ctx);

        /* statement */
        else_ = parse_stmt(ctx);

        /* leave else scope */
        sema_if_stmt_leave_block(ctx);
    }

    /* make node */
    return sema_if_stmt(ctx, t, condition, then, else_);
}

StmtNode *parse_while_stmt(ParserContext *ctx) {
    const Token *t;
    ExprNode *condition;
    StmtNode *body;

    /* while */
    t = consume_token(ctx);

    if (t->kind != token_while) {
        fprintf(stderr, "expected while, but got %s\n", t->text);
        exit(1);
    }

    /* ( expression ) */
    condition = parse_paren_expr(ctx);

    /* enter body scope */
    sema_while_stmt_enter_body(ctx);

    /* statement */
    body = parse_stmt(ctx);

    /* leave body scope and make node */
    return sema_while_stmt_leave_body(ctx, t, condition, body);
}

StmtNode *parse_do_stmt(ParserContext *ctx) {
    const Token *t;
    StmtNode *body;
    ExprNode *condition;

    /* do */
    t = consume_token(ctx);

    if (t->kind != token_do) {
        fprintf(stderr, "expected do, but got %s\n", t->text);
        exit(1);
    }

    /* enter body scope */
    sema_do_stmt_enter_body(ctx);

    /* statement */
    body = parse_stmt(ctx);

    /* leave body scope */
    sema_do_stmt_leave_body(ctx);

    /* while */
    if (current_token(ctx)->kind != token_while) {
        fprintf(stderr, "expected while, but got %s\n",
                current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    /* ( expression ) */
    condition = parse_paren_expr(ctx);

    /* ; */
    if (current_token(ctx)->kind != ';') {
        fprintf(stderr, "expected ;, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    /* leave body scope and make node */
    return sema_do_stmt(ctx, t, body, condition);
}

StmtNode *parse_for_stmt(ParserContext *ctx) {
    const Token *t;
    ExprNode *initialization;
    ExprNode *condition;
    ExprNode *continuation;
    StmtNode *body;

    /* for */
    t = consume_token(ctx);

    if (t->kind != token_for) {
        fprintf(stderr, "expected for, but got %s\n", t->text);
        exit(1);
    }

    /* ( */
    if (current_token(ctx)->kind != '(') {
        fprintf(stderr, "expected (, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    /* initialization expression */
    initialization = NULL;

    if (current_token(ctx)->kind != ';') {
        initialization = parse_expr(ctx);
    }

    /* ; */
    if (current_token(ctx)->kind != ';') {
        fprintf(stderr, "expected ;, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    /* condition expression */
    condition = NULL;

    if (current_token(ctx)->kind != ';') {
        condition = parse_expr(ctx);
    }

    /* ; */
    if (current_token(ctx)->kind != ';') {
        fprintf(stderr, "expected ;, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    /* continuation expression */
    continuation = NULL;

    if (current_token(ctx)->kind != ')') {
        continuation = parse_expr(ctx);
    }

    /* ) */
    if (current_token(ctx)->kind != ')') {
        fprintf(stderr, "expected ), but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    /* enter body scope */
    sema_for_stmt_enter_body(ctx);

    /* statement */
    body = parse_stmt(ctx);

    /* leave body scope and make node */
    return sema_for_stmt_leave_body(ctx, t, initialization, condition,
                                    continuation, body);
}

StmtNode *parse_break_stmt(ParserContext *ctx) {
    const Token *t;

    /* break */
    t = consume_token(ctx);

    if (t->kind != token_break) {
        fprintf(stderr, "expected break, but got %s\n", t->text);
        exit(1);
    }

    /* ; */
    if (current_token(ctx)->kind != ';') {
        fprintf(stderr, "expected ;, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    return sema_break_stmt(ctx, t);
}

StmtNode *parse_continue_stmt(ParserContext *ctx) {
    const Token *t;

    /* continue */
    t = consume_token(ctx);

    if (t->kind != token_continue) {
        fprintf(stderr, "expected continue, but got %s\n", t->text);
        exit(1);
    }

    /* ; */
    if (current_token(ctx)->kind != ';') {
        fprintf(stderr, "expected ;, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    return sema_continue_stmt(ctx, t);
}

StmtNode *parse_decl_stmt(ParserContext *ctx) {
    const Token *t;
    DeclNode *decl;

    /* declaration */
    decl = parse_decl(ctx);

    /* ; */
    t = consume_token(ctx);

    if (t->kind != ';') {
        fprintf(stderr, "expected ;, but got %s\n", t->text);
        exit(1);
    }

    /* make node */
    return sema_decl_stmt(ctx, decl, t);
}

StmtNode *parse_expr_stmt(ParserContext *ctx) {
    ExprNode *expr;
    const Token *t;

    /* expression */
    expr = parse_expr(ctx);

    /* ; */
    t = consume_token(ctx);

    if (t->kind != ';') {
        fprintf(stderr, "expected ;, but got %s\n", t->text);
        exit(1);
    }

    /* make node */
    return sema_expr_stmt(ctx, expr, t);
}

StmtNode *parse_stmt(ParserContext *ctx) {
    assert(ctx);

    switch (current_token(ctx)->kind) {
    case '{':
        return parse_compound_stmt(ctx);

    case token_return:
        return parse_return_stmt(ctx);

    case token_if:
        return parse_if_stmt(ctx);

    case token_while:
        return parse_while_stmt(ctx);

    case token_do:
        return parse_do_stmt(ctx);

    case token_for:
        return parse_for_stmt(ctx);

    case token_break:
        return parse_break_stmt(ctx);

    case token_continue:
        return parse_continue_stmt(ctx);

    default:
        if (is_declaration_specifier_token(ctx, current_token(ctx))) {
            return parse_decl_stmt(ctx);
        }
        return parse_expr_stmt(ctx);
    }
}

DeclNode *parse_typedef(ParserContext *ctx) {
    const Token *t;
    const Token *identifier;
    Type *type;

    /* typedef */
    t = consume_token(ctx);

    if (t->kind != token_typedef) {
        fprintf(stderr, "expected typedef, but got %s\n", t->text);
        exit(1);
    }

    /* type */
    type = parse_type(ctx);

    /* identifier */
    identifier = consume_token(ctx);

    if (identifier->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", identifier->text);
        exit(1);
    }

    /* register type symbol and make node */
    return sema_typedef(ctx, t, type, identifier);
}

DeclNode *parse_var_decl(ParserContext *ctx) {
    Type *type;
    const Token *t;

    /* type */
    type = parse_type(ctx);

    /* ;? */
    if (current_token(ctx)->kind == ';') {
        return NULL;
    }

    /* identifier */
    t = consume_token(ctx);

    if (t->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", t->text);
        exit(1);
    }

    /* register symbol and make node */
    return sema_var_decl(ctx, type, t);
}

DeclNode *parse_decl(ParserContext *ctx) {
    assert(ctx);

    switch (current_token(ctx)->kind) {
    case token_typedef:
        return parse_typedef(ctx);

    default:
        return parse_var_decl(ctx);
    }
}

ParamNode *parse_param(ParserContext *ctx) {
    Type *type;
    const Token *t;

    assert(ctx);

    /* type */
    type = parse_type(ctx);

    /* identifier */
    t = consume_token(ctx);

    if (t->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", t->text);
        exit(1);
    }

    /* register symbol and make node */
    return sema_param(ctx, type, t);
}

DeclNode *parse_top_level_typedef(ParserContext *ctx) {
    DeclNode *decl;

    /* typedef */
    decl = parse_typedef(ctx);

    /* ; */
    if (current_token(ctx)->kind != ';') {
        fprintf(stderr, "expected ;, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    return decl;
}

DeclNode *parse_function(ParserContext *ctx) {
    const Token *t;
    Type *return_type;
    Vec *params;
    StmtNode *body;

    FunctionNode *p;

    /* type */
    return_type = parse_type(ctx);

    /* ;? */
    if (current_token(ctx)->kind == ';') {
        consume_token(ctx);

        return NULL;
    }

    /* identifier */
    t = consume_token(ctx);

    if (t->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", t->text);
        exit(1);
    }

    /* ;? */
    if (current_token(ctx)->kind == ';') {
        consume_token(ctx);

        /* global variable */
        return sema_var_decl(ctx, return_type, t);
    }

    /* ( */
    if (current_token(ctx)->kind != '(') {
        fprintf(stderr, "expected (, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    /* enter parameter scope */
    sema_function_enter_params(ctx);

    /* parameters */
    params = vec_new();

    if (current_token(ctx)->kind == token_void) {
        /* void */
        consume_token(ctx);
    } else {
        /* param {, param} */
        /* param */
        vec_push(params, parse_param(ctx));

        while (current_token(ctx)->kind == ',') {
            /* , */
            consume_token(ctx);

            /* param */
            vec_push(params, parse_param(ctx));
        }
    }

    /* ) */
    if (current_token(ctx)->kind != ')') {
        fprintf(stderr, "expected ), but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx);

    /* leave parameter scope and make node */
    p = sema_function_leave_params(
        ctx, return_type, t, (ParamNode **)params->data, params->size, false);

    if (current_token(ctx)->kind == ';') {
        consume_token(ctx); /* eat ; */

        return (DeclNode *)p;
    }

    /* enter function body */
    sema_function_enter_body(ctx, p);

    /* body */
    body = parse_compound_stmt(ctx);

    /* leave function body */
    return (DeclNode *)sema_function_leave_body(ctx, p, body);
}

DeclNode *parse_top_level(ParserContext *ctx) {
    assert(ctx);

    switch (current_token(ctx)->kind) {
    case token_typedef:
        return parse_top_level_typedef(ctx);

    default:
        return parse_function(ctx);
    }
}

TranslationUnitNode *parse(const char *filename, const char *src) {
    ParserContext *ctx;
    DeclNode *decl;
    Vec *decls;

    assert(filename);
    assert(src);

    /* enter translation unit */
    ctx = sema_translation_unit_enter(src);

    /* top level declarations */
    decls = vec_new();

    while (current_token(ctx)->kind != '\0') {
        decl = parse_top_level(ctx);

        if (decl) {
            vec_push(decls, decl);
        }
    }

    /* make node */
    return sema_translation_unit_leave(ctx, filename, (DeclNode **)decls->data,
                                       decls->size);
}
