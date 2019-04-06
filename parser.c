#include "nocc.h"

const Token *current_token(ParserContext *ctx) {
    assert(ctx);
    return ctx->tokens[ctx->index];
}

const Token *consume_token(ParserContext *ctx) {
    assert(ctx);

    if (current_token(ctx)->kind == '\0') {
        return current_token(ctx);
    }

    return ctx->tokens[ctx->index++];
}

Type *parse_type(ParserContext *ctx) {
    assert(ctx);

    switch (current_token(ctx)->kind) {
    case token_void:
        consume_token(ctx); /* eat void */
        return type_get_void();

    case token_int:
        consume_token(ctx); /* eat int */
        return type_get_int32();

    default:
        fprintf(stderr, "expected type, but got %s\n",
                current_token(ctx)->text);
        exit(1);
    }
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
    int value;

    /* number */
    t = consume_token(ctx);

    if (t->kind != token_number) {
        fprintf(stderr, "expected number, but got %s\n", t->text);
        exit(1);
    }

    /* convert */
    value = atoi(t->text); /* TODO: check value range */

    /* make node */
    return sema_integer_expr(ctx, t, value);
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
        vec_push(args, parse_expr(ctx));

        /* {, expression} */
        while (current_token(ctx)->kind == ',') {
            /* , */
            consume_token(ctx);

            /* expression */
            vec_push(args, parse_expr(ctx));
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

ExprNode *parse_postfix_expr(ParserContext *ctx) {
    ExprNode *operand;

    operand = parse_primary_expr(ctx);

    while (1) {
        switch (current_token(ctx)->kind) {
        case '(':
            operand = parse_call_expr(ctx, operand);
            break;

        default:
            return operand;
        }
    }
}

ExprNode *parse_unary_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *operand;

    t = current_token(ctx);

    switch (t->kind) {
    case '-':
        /* unary operator */
        consume_token(ctx);

        /* unary expression */
        operand = parse_unary_expr(ctx);

        /* make node */
        return sema_unary_expr(ctx, t, operand);

    default:
        return parse_postfix_expr(ctx);
    }
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

ExprNode *parse_expr(ParserContext *ctx) {
    assert(ctx);

    return parse_equality_expr(ctx);
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

    /* statement */
    then = parse_stmt(ctx);

    /* else */
    else_ = NULL;

    if (current_token(ctx)->kind == token_else) {
        consume_token(ctx);

        /* statement */
        else_ = parse_stmt(ctx);
    }

    /* make node */
    return sema_if_stmt(ctx, t, condition, then, else_);
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

    default:
        return parse_expr_stmt(ctx);
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

    /* make node */
    return sema_param(ctx, type, t);
}

DeclNode *parse_top_level(ParserContext *ctx) {
    const Token *t;
    Type *return_type;
    Vec *params;
    StmtNode *body;

    FunctionNode *p;

    assert(ctx);

    /* type */
    return_type = parse_type(ctx);

    /* identifier */
    t = consume_token(ctx);

    if (t->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", t->text);
        exit(1);
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

TranslationUnitNode *parse(const char *filename, const char *src) {
    ParserContext *ctx;
    Vec *decls;

    assert(filename);
    assert(src);

    /* enter translation unit */
    ctx = sema_translation_unit_enter(src);

    /* top level declarations */
    decls = vec_new();

    while (current_token(ctx)->kind != '\0') {
        vec_push(decls, parse_top_level(ctx));
    }

    /* make node */
    return sema_translation_unit_leave(filename, (DeclNode **)decls->data,
                                       decls->size);
}
