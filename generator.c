#include "nocc.h"

LLVMValueRef generate_integer_expr(IntegerNode *p) {
    return LLVMConstInt(LLVMInt32Type(), p->value, true);
}

LLVMValueRef generate_unary_expr(GeneratorContext *ctx, UnaryNode *p) {
    LLVMValueRef operand;

    operand = generate_expr(ctx, p->operand);

    switch (p->operator_) {
    case '-':
        return LLVMBuildNeg(ctx->builder, operand, "neg");

    default:
        fprintf(stderr, "unknown unary operator %d\n", p->operator_);
        exit(1);
    }
}

LLVMValueRef generate_binary_expr(GeneratorContext *ctx, BinaryNode *p) {
    LLVMValueRef left;
    LLVMValueRef right;

    left = generate_expr(ctx, p->left);
    right = generate_expr(ctx, p->right);

    switch (p->operator_) {
    case '+':
        return LLVMBuildAdd(ctx->builder, left, right, "add");

    case '-':
        return LLVMBuildSub(ctx->builder, left, right, "sub");

    case '*':
        return LLVMBuildMul(ctx->builder, left, right, "mul");

    case '/':
        return LLVMBuildSDiv(ctx->builder, left, right, "div");

    case '%':
        return LLVMBuildSRem(ctx->builder, left, right, "rem");

    default:
        fprintf(stderr, "unknown binary operator %d\n", p->operator_);
        exit(1);
    }
}

LLVMValueRef generate_expr(GeneratorContext *ctx, ExprNode *p) {
    assert(p);
    assert(ctx);

    switch (p->kind) {
    case node_integer:
        return generate_integer_expr((IntegerNode *)p);

    case node_unary:
        return generate_unary_expr(ctx, (UnaryNode *)p);

    case node_binary:
        return generate_binary_expr(ctx, (BinaryNode *)p);

    default:
        fprintf(stderr, "unknown expression %d\n", p->kind);
        exit(1);
    }
}
