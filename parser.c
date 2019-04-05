#include "nocc.h"

Node *parse_number_expr(const Token **toks, int *n) {
    IntegerNode *p;

    if (toks[*n]->kind != token_number) {
        fprintf(stderr, "expected number, but got %s\n", toks[*n]->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_integer;
    p->line = toks[*n]->line;
    p->value = atoi(toks[*n]->text);

    *n += 1;

    return (Node *)p;
}

Node *parse_identifier_expr(const Token **toks, int *n) {
    IdentifierNode *p;

    if (toks[*n]->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s\n", toks[*n]->text);
        exit(1);
    }

    p = malloc(sizeof(*p));
    p->kind = node_identifier;
    p->line = toks[*n]->line;
    p->identifier = strdup(toks[*n]->text);

    *n += 1;

    return (Node *)p;
}

Node *parse_primary_expr(const Token **toks, int *n) {
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

Node *parse_unary_expr(const Token **toks, int *n) {
    UnaryNode *p;

    switch (toks[*n]->kind) {
    case '-':
        p = malloc(sizeof(*p));
        p->kind = node_unary;
        p->line = toks[*n]->line;
        p->operator_ = toks[*n]->kind;

        *n += 1;
        p->operand = parse_unary_expr(toks, n);

        return (Node *)p;

    default:
        return parse_primary_expr(toks, n);
    }
}

Node *parse_multiplicative_expr(const Token **toks, int *n) {
    BinaryNode *p;
    Node *left;

    left = parse_unary_expr(toks, n);

    while (toks[*n]->kind == '*' || toks[*n]->kind == '/' ||
           toks[*n]->kind == '%') {
        p = malloc(sizeof(*p));
        p->kind = node_binary;
        p->line = toks[*n]->line;
        p->operator_ = toks[*n]->kind;
        p->left = left;

        *n += 1;

        p->right = parse_unary_expr(toks, n);

        left = (Node *)p;
    }

    return left;
}

Node *parse_additive_expr(const Token **toks, int *n) {
    BinaryNode *p;
    Node *left;

    left = parse_multiplicative_expr(toks, n);

    while (toks[*n]->kind == '+' || toks[*n]->kind == '-') {
        p = malloc(sizeof(*p));
        p->kind = node_binary;
        p->line = toks[*n]->line;
        p->operator_ = toks[*n]->kind;
        p->left = left;

        *n += 1;

        p->right = parse_multiplicative_expr(toks, n);

        left = (Node *)p;
    }

    return left;
}

Node *parse_expr(const Token **toks, int *n) {
    assert(toks);
    assert(toks[0]);
    assert(n);
    assert(*n >= 0);

    return parse_additive_expr(toks, n);
}
