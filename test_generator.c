#include "nocc.h"

void test_generating_type_void(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
        .builder = LLVMCreateBuilder(),
    };

    assert(generate_type(ctx, type_get_void()) == LLVMVoidType());

    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_type_int32(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
        .builder = LLVMCreateBuilder(),
    };

    assert(generate_type(ctx, type_get_int32()) == LLVMInt32Type());

    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_integer(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
        .builder = LLVMCreateBuilder(),
    };

    IntegerNode *p = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .value = 42,
    };

    LLVMValueRef v = generate_expr(ctx, (ExprNode *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 42);

    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_negative(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
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
        .operand = (ExprNode *)q,
    };

    LLVMValueRef v = generate_expr(ctx, (ExprNode *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == -42);

    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_addition(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
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
        .left = (ExprNode *)l,
        .right = (ExprNode *)r,
    };

    LLVMValueRef v = generate_expr(ctx, (ExprNode *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 23);

    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_subtraction(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
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
        .left = (ExprNode *)l,
        .right = (ExprNode *)r,
    };

    LLVMValueRef v = generate_expr(ctx, (ExprNode *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 7);

    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_multiplication(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
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
        .left = (ExprNode *)l,
        .right = (ExprNode *)r,
    };

    LLVMValueRef v = generate_expr(ctx, (ExprNode *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 120);

    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_division(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
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
        .left = (ExprNode *)l,
        .right = (ExprNode *)r,
    };

    LLVMValueRef v = generate_expr(ctx, (ExprNode *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 1);

    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_modulo(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
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
        .left = (ExprNode *)l,
        .right = (ExprNode *)r,
    };

    LLVMValueRef v = generate_expr(ctx, (ExprNode *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 7);

    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_function_prototype(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
        .builder = LLVMCreateBuilder(),
    };

    FunctionNode *p = &(FunctionNode){
        .kind = node_function,
        .line = 1,
        .identifier = "f",
        .return_type = type_get_int32(),
        .params = vec_new(),
        .var_args = false,
        .body = NULL,
    };

    LLVMValueRef function = generate_function(ctx, p);
    char *message = LLVMPrintValueToString(function);

    assert(strcmp(message, "\n"
                           "declare i32 @f()\n") == 0);

    LLVMDisposeMessage(message);
    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_function(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
        .builder = LLVMCreateBuilder(),
    };

    FunctionNode *p = &(FunctionNode){
        .kind = node_function,
        .line = 1,
        .identifier = "f",
        .return_type = type_get_void(),
        .params = vec_new(),
        .var_args = false,
        .body =
            (StmtNode *)&(CompoundNode){
                .kind = node_compound,
                .line = 1,
                .stmts = vec_new(),
            },
    };

    LLVMValueRef function = generate_function(ctx, p);
    char *message = LLVMPrintValueToString(function);

    assert(strcmp(message, "\n"
                           "define void @f() {\n"
                           "entry:\n"
                           "  ret void\n"
                           "}\n") == 0);

    LLVMDisposeMessage(message);
    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_translation_unit(void) {
    TranslationUnitNode *p = parse("test_generating_translation_unit",
                                   "int main(void) {return 42;}");

    LLVMModuleRef module = generate(p);

    char *message = LLVMPrintModuleToString(module);

    assert(strcmp(message,
                  "; ModuleID = 'test_generating_translation_unit'\n"
                  "source_filename = \"test_generating_translation_unit\"\n"
                  "\n"
                  "define i32 @main() {\n"
                  "entry:\n"
                  "  ret i32 42\n"
                  "}\n") == 0);

    LLVMDisposeMessage(message);
    LLVMDisposeModule(module);
}

void test_generator(void) {
    test_generating_type_void();
    test_generating_type_int32();

    test_generating_integer();
    test_generating_negative();
    test_generating_addition();
    test_generating_subtraction();
    test_generating_multiplication();
    test_generating_division();
    test_generating_modulo();

    test_generating_function_prototype();
    test_generating_function();

    test_generating_translation_unit();
}
