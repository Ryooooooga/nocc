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

ExprNode *parse_number_expr(const Token **toks, int *n) {
    IntegerNode *p;

    if (toks[*n]->kind != token_number) {
        fprintf(stderr, "expected number, but got %s\n", toks[*n]->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_integer;
    p->line = toks[*n]->line;
    p->value = atoi(toks[*n]->text);

    *n += 1; /* eat number */

    return (ExprNode *)p;
}

ExprNode *parse_identifier_expr(const Token **toks, int *n) {
    IdentifierNode *p;

    if (toks[*n]->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", toks[*n]->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_identifier;
    p->line = toks[*n]->line;
    p->identifier = strdup(toks[*n]->text);

    *n += 1; /* eat identifier */

    return (ExprNode *)p;
}

ExprNode *parse_primary_expr(const Token **toks, int *n) {
    switch (toks[*n]->kind) {
    case token_number:
        return parse_number_expr(toks, n);

    case token_identifier:
        return parse_identifier_expr(toks, n);

    default:
        fprintf(stderr, "expected expression, but got %s\n", toks[*n]->text);
        exit(1);
    }
}

ExprNode *parse_unary_expr(const Token **toks, int *n) {
    UnaryNode *p;

    switch (toks[*n]->kind) {
    case '-':
        p = malloc(sizeof(*p));
        p->kind = node_unary;
        p->line = toks[*n]->line;
        p->operator_ = toks[*n]->kind;

        *n += 1; /* eat unary operator */
        p->operand = parse_unary_expr(toks, n);

        return (ExprNode *)p;

    default:
        return parse_primary_expr(toks, n);
    }
}

ExprNode *parse_multiplicative_expr(const Token **toks, int *n) {
    const Token *op_tok;
    ExprNode *left;
    ExprNode *right;

    left = parse_unary_expr(toks, n);

    while (toks[*n]->kind == '*' || toks[*n]->kind == '/' ||
           toks[*n]->kind == '%') {
        op_tok = toks[*n];
        *n += 1; /* eat binary operator */

        right = parse_unary_expr(toks, n);

        left = binary_expr_new(op_tok, left, right);
    }

    return left;
}

ExprNode *parse_additive_expr(const Token **toks, int *n) {
    const Token *op_tok;
    ExprNode *left;
    ExprNode *right;

    left = parse_multiplicative_expr(toks, n);

    while (toks[*n]->kind == '+' || toks[*n]->kind == '-') {
        op_tok = toks[*n];
        *n += 1; /* eat binary operator */

        right = parse_multiplicative_expr(toks, n);

        left = binary_expr_new(op_tok, left, right);
    }

    return left;
}

ExprNode *parse_expr(const Token **toks, int *n) {
    assert(toks);
    assert(toks[0]);
    assert(n);
    assert(*n >= 0);

    return parse_additive_expr(toks, n);
}

StmtNode *parse_compound_stmt(const Token **toks, int *n) {
    CompoundStmtNode *p;

    if (toks[*n]->kind != '{') {
        fprintf(stderr, "expected {, but got %s\n", toks[*n]->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_compound;
    p->line = toks[*n]->line;
    p->stmts = vec_new();

    *n += 1; /* eat { */

    while (toks[*n]->kind != '}') {
        vec_push(p->stmts, parse_stmt(toks, n));
    }

    if (toks[*n]->kind != '}') {
        fprintf(stderr, "expected }, but got %s\n", toks[*n]->text);
        exit(1);
    }

    return (StmtNode *)p;
}

StmtNode *parse_return_stmt(const Token **toks, int *n) {
    ReturnStmtNode *p;

    if (toks[*n]->kind != token_return) {
        fprintf(stderr, "expected return, but got %s\n", toks[*n]->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_return;
    p->line = toks[*n]->line;
    p->return_value = NULL;

    *n += 1; /* eat return */

    if (toks[*n]->kind != ';') {
        p->return_value = parse_expr(toks, n);
    }

    if (toks[*n]->kind != ';') {
        fprintf(stderr, "expected semicolon, but got %s\n", toks[*n]->text);
        exit(1);
    }

    *n += 1; /* eat ; */

    return (StmtNode *)p;
}

StmtNode *parse_expr_stmt(const Token **toks, int *n) {
    ExprNode *expr;
    ExprStmtNode *p;

    expr = parse_expr(toks, n);

    p = malloc(sizeof(*p));
    p->kind = node_expr;
    p->expr = expr;
    p->line = toks[*n]->line;

    *n += 1; /* eat ; */

    return (StmtNode *)p;
}

StmtNode *parse_stmt(const Token **toks, int *n) {
    assert(toks);
    assert(toks[0]);
    assert(n);
    assert(*n >= 0);

    switch (toks[*n]->kind) {
    case '{':
        return parse_compound_stmt(toks, n);

    case token_return:
        return parse_return_stmt(toks, n);

    default:
        return parse_expr_stmt(toks, n);
    }
}
