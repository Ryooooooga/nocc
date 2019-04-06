#include "nocc.h"

DeclNode *scope_get(ParserContext *ctx, const char *name) {
    return map_get(ctx->env, name);
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
    p->identifier = strdup(t->text);
    p->declaration = scope_get(ctx, p->identifier);

    if (p->declaration == NULL) {
        fprintf(stderr, "undeclared symbol %s\n", p->identifier);
        exit(1);
    }

    p->type = p->declaration->type;

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
    p->operator_ = t->kind;
    p->left = left;
    p->right = right;

    switch (t->kind) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
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
        if (left->type != type_get_int32() || right->type != type_get_int32()) {
            fprintf(stderr, "invalid operand type of binary operator %s\n",
                    t->text);
            exit(1);
        }

        p->type = type_get_int32();
        break;

    default:
        fprintf(stderr, "unknown binary operator %s\n", t->text);
        exit(1);
    }

    return (ExprNode *)p;
}
