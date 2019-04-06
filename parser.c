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

Type *parse_type(ParserContext *ctx) {
    assert(ctx);

    switch (ctx->tokens[ctx->index]->kind) {
    case token_void:
        ctx->index += 1; /* eat void */
        return type_get_void();

    case token_int:
        ctx->index += 1; /* eat int */
        return type_get_int32();

    default:
        fprintf(stderr, "expected type, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }
}

ExprNode *parse_number_expr(ParserContext *ctx) {
    IntegerNode *p;

    if (ctx->tokens[ctx->index]->kind != token_number) {
        fprintf(stderr, "expected number, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_integer;
    p->line = ctx->tokens[ctx->index]->line;
    p->value = atoi(ctx->tokens[ctx->index]->text);

    ctx->index += 1; /* eat number */

    return (ExprNode *)p;
}

ExprNode *parse_identifier_expr(ParserContext *ctx) {
    IdentifierNode *p;

    if (ctx->tokens[ctx->index]->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_identifier;
    p->line = ctx->tokens[ctx->index]->line;
    p->identifier = strdup(ctx->tokens[ctx->index]->text);
    p->declaration = map_get(ctx->env, p->identifier);

    if (p->declaration == NULL) {
        fprintf(stderr, "undeclared symbol %s\n", p->identifier);
        exit(1);
    }

    ctx->index += 1; /* eat identifier */

    return (ExprNode *)p;
}

ExprNode *parse_primary_expr(ParserContext *ctx) {
    switch (ctx->tokens[ctx->index]->kind) {
    case token_number:
        return parse_number_expr(ctx);

    case token_identifier:
        return parse_identifier_expr(ctx);

    default:
        fprintf(stderr, "expected expression, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }
}

ExprNode *parse_unary_expr(ParserContext *ctx) {
    UnaryNode *p;

    switch (ctx->tokens[ctx->index]->kind) {
    case '-':
        p = malloc(sizeof(*p));
        p->kind = node_unary;
        p->line = ctx->tokens[ctx->index]->line;
        p->operator_ = ctx->tokens[ctx->index]->kind;

        ctx->index += 1; /* eat unary operator */
        p->operand = parse_unary_expr(ctx);

        return (ExprNode *)p;

    default:
        return parse_primary_expr(ctx);
    }
}

ExprNode *parse_multiplicative_expr(ParserContext *ctx) {
    const Token *op_tok;
    ExprNode *left;
    ExprNode *right;

    left = parse_unary_expr(ctx);

    while (ctx->tokens[ctx->index]->kind == '*' ||
           ctx->tokens[ctx->index]->kind == '/' ||
           ctx->tokens[ctx->index]->kind == '%') {
        op_tok = ctx->tokens[ctx->index];
        ctx->index += 1; /* eat binary operator */

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

    while (ctx->tokens[ctx->index]->kind == '+' ||
           ctx->tokens[ctx->index]->kind == '-') {
        op_tok = ctx->tokens[ctx->index];
        ctx->index += 1; /* eat binary operator */

        right = parse_multiplicative_expr(ctx);

        left = binary_expr_new(op_tok, left, right);
    }

    return left;
}

ExprNode *parse_expr(ParserContext *ctx) {
    assert(ctx);

    return parse_additive_expr(ctx);
}

StmtNode *parse_compound_stmt(ParserContext *ctx) {
    CompoundNode *p;

    if (ctx->tokens[ctx->index]->kind != '{') {
        fprintf(stderr, "expected {, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_compound;
    p->line = ctx->tokens[ctx->index]->line;
    p->stmts = vec_new();

    ctx->index += 1; /* eat { */

    /* TODO: lexical scope */

    while (ctx->tokens[ctx->index]->kind != '}') {
        vec_push(p->stmts, parse_stmt(ctx));
    }

    if (ctx->tokens[ctx->index]->kind != '}') {
        fprintf(stderr, "expected }, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }

    ctx->index += 1; /* eat } */

    return (StmtNode *)p;
}

StmtNode *parse_return_stmt(ParserContext *ctx) {
    ReturnNode *p;

    if (ctx->tokens[ctx->index]->kind != token_return) {
        fprintf(stderr, "expected return, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_return;
    p->line = ctx->tokens[ctx->index]->line;
    p->return_value = NULL;

    ctx->index += 1; /* eat return */

    if (ctx->tokens[ctx->index]->kind != ';') {
        p->return_value = parse_expr(ctx);
    }

    if (ctx->tokens[ctx->index]->kind != ';') {
        fprintf(stderr, "expected semicolon, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }

    ctx->index += 1; /* eat ; */

    return (StmtNode *)p;
}

StmtNode *parse_expr_stmt(ParserContext *ctx) {
    ExprNode *expr;
    ExprStmtNode *p;

    expr = parse_expr(ctx);

    p = malloc(sizeof(*p));
    p->kind = node_expr;
    p->expr = expr;
    p->line = ctx->tokens[ctx->index]->line;

    ctx->index += 1; /* eat ; */

    return (StmtNode *)p;
}

StmtNode *parse_stmt(ParserContext *ctx) {
    assert(ctx);

    switch (ctx->tokens[ctx->index]->kind) {
    case '{':
        return parse_compound_stmt(ctx);

    case token_return:
        return parse_return_stmt(ctx);

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

    if (ctx->tokens[ctx->index]->kind != token_identifier) {
        fprintf(stderr, "identifier is expected, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }

    identifier = ctx->tokens[ctx->index];

    ctx->index += 1; /* eat identifier */

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

    if (ctx->tokens[ctx->index]->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }

    identifier = ctx->tokens[ctx->index];
    ctx->index += 1; /* eat identifier */

    if (ctx->tokens[ctx->index]->kind != '(') {
        fprintf(stderr, "expected (, but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }
    ctx->index += 1; /* eat ( */

    params = vec_new();

    if (ctx->tokens[ctx->index]->kind == token_void) {
        ctx->index += 1; /* eat void */
    } else {
        vec_push(params, parse_param(ctx));

        while (ctx->tokens[ctx->index]->kind == ',') {
            ctx->index += 1; /* eat , */

            vec_push(params, parse_param(ctx));
        }
    }

    if (ctx->tokens[ctx->index]->kind != ')') {
        fprintf(stderr, "expected ), but got %s\n",
                ctx->tokens[ctx->index]->text);
        exit(1);
    }
    ctx->index += 1; /* eat ) */

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

    if (ctx->tokens[ctx->index]->kind == ';') {
        ctx->index += 1; /* eat ; */
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
