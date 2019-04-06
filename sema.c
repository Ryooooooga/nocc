#include "nocc.h"

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
    p->declaration = scope_stack_find(ctx->env, p->identifier, true);

    if (p->declaration == NULL) {
        fprintf(stderr, "undeclared symbol %s\n", p->identifier);
        exit(1);
    }

    p->type = p->declaration->type;

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
        /* TODO: type limitation */
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

    assert(ctx);
    assert(t);
    assert(semi);

    p = malloc(sizeof(*p));
    p->kind = node_return;
    p->line = t->line;
    p->return_value = return_value;

    /* TODO: type check */

    return (StmtNode *)p;
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

ParserContext *sema_translation_unit_enter(const char *src) {
    ParserContext *ctx;
    Vec *tokens;

    assert(src);

    tokens = lex(src);

    ctx = malloc(sizeof(*ctx));
    ctx->env = scope_stack_new();
    ctx->tokens = (const Token **)tokens->data;
    ctx->index = 0;

    return ctx;
}

TranslationUnitNode *sema_translation_unit_leave(const char *filename,
                                                 DeclNode **decls,
                                                 int num_decls) {
    TranslationUnitNode *p;
    int i;

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
