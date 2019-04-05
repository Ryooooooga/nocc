#include "nocc.h"

void test_generating_integer(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .builder = LLVMCreateBuilder(),
    };

    IntegerNode *p = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 42,
    };

    LLVMValueRef v = generate_expr(ctx, (Node *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 42);

    LLVMDisposeBuilder(ctx->builder);
}

void test_generating_negative(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .builder = LLVMCreateBuilder(),
    };

    IntegerNode *q = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 42,
    };

    UnaryNode *p = &(UnaryNode){
        .kind = node_unary,
        .line = 1,
        .operator_ = '-',
        .operand = (Node *)q,
    };

    LLVMValueRef v = generate_expr(ctx, (Node *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == -42);

    LLVMDisposeBuilder(ctx->builder);
}

void test_generator(void) {
    test_generating_integer();
    test_generating_negative();
}
