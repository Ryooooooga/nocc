#include "nocc.h"

LLVMTypeRef generate_type(GeneratorContext *ctx, Type *p) {
    assert(ctx);
    assert(p);

    (void)ctx;

    switch (p->kind) {
    case type_void:
        return LLVMVoidType();

    case type_int32:
        return LLVMInt32Type();

    default:
        fprintf(stderr, "unknown type %d\n", p->kind);
        exit(1);
    }
}

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

LLVMValueRef generate_function(GeneratorContext *ctx, FunctionNode *p) {
    LLVMTypeRef return_type;
    LLVMTypeRef *param_types;
    LLVMTypeRef function_type;
    LLVMValueRef function;
    int i;

    assert(ctx);
    assert(p);

    return_type = generate_type(ctx, p->return_type);

    param_types = malloc(sizeof(LLVMTypeRef) * p->params->size);

    for (i = 0; i < p->params->size; i++) {
        // TODO: params
    }

    function_type = LLVMFunctionType(return_type, param_types, p->params->size,
                                     p->var_args);

    function = LLVMAddFunction(ctx->module, p->identifier, function_type);

    if (p->body == NULL) {
        return function;
    }

    // TODO: body

    return function;
}

void generate_decl(GeneratorContext *ctx, DeclNode *p) {
    assert(p);
    assert(ctx);

    switch (p->kind) {
    case node_function:
        generate_function(ctx, (FunctionNode *)p);
        return;

    default:
        fprintf(stderr, "unknown declaration %d\n", p->kind);
        exit(1);
    }
}
