#include "nocc.h"

ExprNode *binary_expr_new(const Token *op_tok, ExprNode *left,
                          ExprNode *right) {
    BinaryNode *p;

    p = malloc(sizeof(*p));
    p->kind = node_binary;
    p->line = op_tok->line;
    p->operator_ = op_tok->kind;
    p->left = left;
    p->right = right;

    return (ExprNode *)p;
}

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
    CallNode *p;

    p = malloc(sizeof(*p));
    p->kind = node_call;
    p->line = current_token(ctx)->line;
    p->callee = callee;
    p->args = vec_new();

    if (current_token(ctx)->kind != '(') {
        fprintf(stderr, "expected (, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx); /* eat ( */

    if (current_token(ctx)->kind == ')') {
        consume_token(ctx); /* eat ) */

        return (ExprNode *)p;
    }

    vec_push(p->args, parse_expr(ctx));

    while (current_token(ctx)->kind == ',') {
        consume_token(ctx); /* eat , */

        vec_push(p->args, parse_expr(ctx));
    }

    if (current_token(ctx)->kind != ')') {
        fprintf(stderr, "expected ), but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx); /* eat ) */

    return (ExprNode *)p;
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
    UnaryNode *p;

    switch (current_token(ctx)->kind) {
    case '-':
        p = malloc(sizeof(*p));
        p->kind = node_unary;
        p->line = current_token(ctx)->line;
        p->operator_ = current_token(ctx)->kind;

        consume_token(ctx); /* eat unary operator */
        p->operand = parse_unary_expr(ctx);

        return (ExprNode *)p;

    default:
        return parse_postfix_expr(ctx);
    }
}

ExprNode *parse_multiplicative_expr(ParserContext *ctx) {
    const Token *op_tok;
    ExprNode *left;
    ExprNode *right;

    left = parse_unary_expr(ctx);

    while (current_token(ctx)->kind == '*' || current_token(ctx)->kind == '/' ||
           current_token(ctx)->kind == '%') {
        op_tok = current_token(ctx);
        consume_token(ctx); /* eat binary operator */

        right = parse_unary_expr(ctx);

        left = binary_expr_new(op_tok, left, right);
    }

    return left;
}

ExprNode *parse_additive_expr(ParserContext *ctx) {
    const Token *op_tok;
    ExprNode *left;
    ExprNode *right;

    left = parse_multiplicative_expr(ctx);

    while (current_token(ctx)->kind == '+' || current_token(ctx)->kind == '-') {
        op_tok = current_token(ctx);
        consume_token(ctx); /* eat binary operator */

        right = parse_multiplicative_expr(ctx);

        left = binary_expr_new(op_tok, left, right);
    }

    return left;
}

ExprNode *parse_relational_expr(ParserContext *ctx) {
    const Token *op_tok;
    ExprNode *left;
    ExprNode *right;

    left = parse_additive_expr(ctx);

    while (current_token(ctx)->kind == '<' || current_token(ctx)->kind == '>' ||
           current_token(ctx)->kind == token_lesser_equal ||
           current_token(ctx)->kind == token_greater_equal) {
        op_tok = current_token(ctx);
        consume_token(ctx); /* eat binary operator */

        right = parse_additive_expr(ctx);

        left = binary_expr_new(op_tok, left, right);
    }

    return left;
}

ExprNode *parse_equality_expr(ParserContext *ctx) {
    const Token *op_tok;
    ExprNode *left;
    ExprNode *right;

    left = parse_relational_expr(ctx);

    while (current_token(ctx)->kind == token_equal ||
           current_token(ctx)->kind == token_not_equal) {
        op_tok = current_token(ctx);
        consume_token(ctx); /* eat binary operator */

        right = parse_relational_expr(ctx);

        left = binary_expr_new(op_tok, left, right);
    }

    return left;
}

ExprNode *parse_expr(ParserContext *ctx) {
    assert(ctx);

    return parse_equality_expr(ctx);
}

StmtNode *parse_compound_stmt(ParserContext *ctx) {
    CompoundNode *p;

    if (current_token(ctx)->kind != '{') {
        fprintf(stderr, "expected {, but got %s\n", current_token(ctx)->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_compound;
    p->line = current_token(ctx)->line;
    p->stmts = vec_new();

    consume_token(ctx); /* eat { */

    /* TODO: lexical scope */

    while (current_token(ctx)->kind != '}') {
        vec_push(p->stmts, parse_stmt(ctx));
    }

    if (current_token(ctx)->kind != '}') {
        fprintf(stderr, "expected }, but got %s\n", current_token(ctx)->text);
        exit(1);
    }

    consume_token(ctx); /* eat } */

    return (StmtNode *)p;
}

StmtNode *parse_return_stmt(ParserContext *ctx) {
    ReturnNode *p;

    if (current_token(ctx)->kind != token_return) {
        fprintf(stderr, "expected return, but got %s\n",
                current_token(ctx)->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_return;
    p->line = current_token(ctx)->line;
    p->return_value = NULL;

    consume_token(ctx); /* eat return */

    if (current_token(ctx)->kind != ';') {
        p->return_value = parse_expr(ctx);
    }

    if (current_token(ctx)->kind != ';') {
        fprintf(stderr, "expected semicolon, but got %s\n",
                current_token(ctx)->text);
        exit(1);
    }

    consume_token(ctx); /* eat ; */

    return (StmtNode *)p;
}

StmtNode *parse_if_stmt(ParserContext *ctx) {
    IfNode *p;

    p = malloc(sizeof(*p));
    p->kind = node_if;
    p->line = current_token(ctx)->line;
    p->else_ = NULL;

    if (current_token(ctx)->kind != token_if) {
        fprintf(stderr, "expected if, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx); /* eat if */

    p->condition = parse_paren_expr(ctx);

    p->then = parse_stmt(ctx);

    if (current_token(ctx)->kind != token_else) {
        return (StmtNode *)p;
    }
    consume_token(ctx); // eat else

    p->else_ = parse_stmt(ctx);

    return (StmtNode *)p;
}

StmtNode *parse_expr_stmt(ParserContext *ctx) {
    ExprNode *expr;
    ExprStmtNode *p;

    expr = parse_expr(ctx);

    p = malloc(sizeof(*p));
    p->kind = node_expr;
    p->expr = expr;
    p->line = current_token(ctx)->line;

    consume_token(ctx); /* eat ; */

    return (StmtNode *)p;
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
    const Token *identifier;
    ParamNode *p;

    assert(ctx);

    type = parse_type(ctx);

    /* TODO: type check */

    if (current_token(ctx)->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n",
                current_token(ctx)->text);
        exit(1);
    }

    identifier = current_token(ctx);

    consume_token(ctx); /* eat identifier */

    p = malloc(sizeof(*p));
    p->kind = node_param;
    p->line = identifier->line;
    p->identifier = strdup(identifier->text);
    p->type = type;
    p->generated_location = NULL;

    return p;
}

DeclNode *parse_top_level(ParserContext *ctx) {
    Type *return_type;
    Type **param_types;
    const Token *identifier;
    Vec *params;
    ParamNode *param;
    FunctionNode *p;
    int env_size;
    int i;

    assert(ctx);

    return_type = parse_type(ctx);

    if (current_token(ctx)->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n",
                current_token(ctx)->text);
        exit(1);
    }

    identifier = current_token(ctx);
    consume_token(ctx); /* eat identifier */

    if (current_token(ctx)->kind != '(') {
        fprintf(stderr, "expected (, but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx); /* eat ( */

    params = vec_new();

    if (current_token(ctx)->kind == token_void) {
        consume_token(ctx); /* eat void */
    } else {
        vec_push(params, parse_param(ctx));

        while (current_token(ctx)->kind == ',') {
            consume_token(ctx); /* eat , */

            vec_push(params, parse_param(ctx));
        }
    }

    if (current_token(ctx)->kind != ')') {
        fprintf(stderr, "expected ), but got %s\n", current_token(ctx)->text);
        exit(1);
    }
    consume_token(ctx); /* eat ) */

    /* make function type */
    param_types = malloc(sizeof(Type *) * params->size);

    for (i = 0; i < params->size; i++) {
        param = params->data[i];
        param_types[i] = param->type;
    }

    p = malloc(sizeof(*p));
    p->kind = node_function;
    p->line = identifier->line;
    p->identifier = strdup(identifier->text);
    p->type = function_type_new(return_type, param_types, params->size);
    p->params = params;
    p->var_args = false;
    p->body = NULL;

    /* register function symbol */
    if (map_contains(ctx->env, p->identifier)) {
        fprintf(stderr, "function %s has already been declared\n",
                p->identifier);
        exit(1);
    }

    map_add(ctx->env, p->identifier, p);
    env_size = map_size(ctx->env);

    if (current_token(ctx)->kind == ';') {
        consume_token(ctx); /* eat ; */
        return (DeclNode *)p;
    }

    /* register parameter symbols */
    for (i = 0; i < params->size; i++) {
        param = params->data[i];

        /* TODO: redeclaration check */
        map_add(ctx->env, param->identifier, param);
    }

    /* parse function body */
    p->body = parse_compound_stmt(ctx);

    map_shrink(ctx->env, env_size);

    return (DeclNode *)p;
}

TranslationUnitNode *parse(const char *filename, const char *src) {
    TranslationUnitNode *p;
    ParserContext ctx;
    Vec *tokens;

    assert(filename);
    assert(src);

    p = malloc(sizeof(*p));
    p->filename = strdup(filename);
    p->decls = vec_new();

    tokens = lex(src);

    ctx.env = map_new();
    ctx.tokens = (const Token **)tokens->data;
    ctx.index = 0;

    while (ctx.tokens[ctx.index]->kind != '\0') {
        vec_push(p->decls, parse_top_level(&ctx));
    }

    return p;
}
