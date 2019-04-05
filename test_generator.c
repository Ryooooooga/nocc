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

void test_generating_addition(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .builder = LLVMCreateBuilder(),
    };

    IntegerNode *l = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 15,
    };

    IntegerNode *r = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 8,
    };

    BinaryNode *p = &(BinaryNode){
        .kind = node_binary,
        .line = 1,
        .operator_ = '+',
        .left = (Node *)l,
        .right = (Node *)r,
    };

    LLVMValueRef v = generate_expr(ctx, (Node *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 23);

    LLVMDisposeBuilder(ctx->builder);
}

void test_generating_subtraction(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .builder = LLVMCreateBuilder(),
    };

    IntegerNode *l = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 15,
    };

    IntegerNode *r = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 8,
    };

    BinaryNode *p = &(BinaryNode){
        .kind = node_binary,
        .line = 1,
        .operator_ = '-',
        .left = (Node *)l,
        .right = (Node *)r,
    };

    LLVMValueRef v = generate_expr(ctx, (Node *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 7);

    LLVMDisposeBuilder(ctx->builder);
}

void test_generating_multiplication(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .builder = LLVMCreateBuilder(),
    };

    IntegerNode *l = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 15,
    };

    IntegerNode *r = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 8,
    };

    BinaryNode *p = &(BinaryNode){
        .kind = node_binary,
        .line = 1,
        .operator_ = '*',
        .left = (Node *)l,
        .right = (Node *)r,
    };

    LLVMValueRef v = generate_expr(ctx, (Node *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 120);

    LLVMDisposeBuilder(ctx->builder);
}

void test_generating_division(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .builder = LLVMCreateBuilder(),
    };

    IntegerNode *l = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 15,
    };

    IntegerNode *r = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 8,
    };

    BinaryNode *p = &(BinaryNode){
        .kind = node_binary,
        .line = 1,
        .operator_ = '/',
        .left = (Node *)l,
        .right = (Node *)r,
    };

    LLVMValueRef v = generate_expr(ctx, (Node *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 1);

    LLVMDisposeBuilder(ctx->builder);
}

void test_generating_modulo(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .builder = LLVMCreateBuilder(),
    };

    IntegerNode *l = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 15,
    };

    IntegerNode *r = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 8,
    };

    BinaryNode *p = &(BinaryNode){
        .kind = node_binary,
        .line = 1,
        .operator_ = '%',
        .left = (Node *)l,
        .right = (Node *)r,
    };

    LLVMValueRef v = generate_expr(ctx, (Node *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 7);

    LLVMDisposeBuilder(ctx->builder);
}

void test_generator(void) {
    test_generating_integer();
    test_generating_negative();
    test_generating_addition();
    test_generating_subtraction();
    test_generating_multiplication();
    test_generating_division();
    test_generating_modulo();
}
