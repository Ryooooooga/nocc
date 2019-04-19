#ifndef USE_STANDARD_HEADERS
#define USE_STANDARD_HEADERS
#endif

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
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 42,
    };

    LLVMValueRef v = generate_expr(ctx, (ExprNode *)p);

    assert(LLVMGetValueKind(v) == LLVMConstantIntValueKind);
    assert(LLVMConstIntGetSExtValue(v) == 42);

    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_identifier(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
        .builder = LLVMCreateBuilder(),
    };

    VariableNode *decl = &(VariableNode){
        .kind = node_variable,
        .line = 1,
        .identifier = "a",
        .type = type_get_int32(),
    };

    IdentifierNode *p = &(IdentifierNode){
        .kind = node_identifier,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = true,
        .identifier = "a",
        .declaration = (DeclNode *)decl,
    };

    LLVMValueRef func = LLVMAddFunction(
        ctx->module, "f", LLVMFunctionType(LLVMInt32Type(), NULL, 0, false));
    LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, block);

    decl->generated_location =
        LLVMBuildAlloca(ctx->builder, LLVMInt32Type(), "a");
    LLVMValueRef v = generate_expr(ctx, (ExprNode *)p);

    assert(LLVMGetInstructionOpcode(v) == LLVMLoad);

    LLVMBuildRet(ctx->builder, v);

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
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 42,
    };

    UnaryNode *p = &(UnaryNode){
        .kind = node_unary,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
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
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 15,
    };

    IntegerNode *r = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 8,
    };

    BinaryNode *p = &(BinaryNode){
        .kind = node_binary,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
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
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 15,
    };

    IntegerNode *r = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 8,
    };

    BinaryNode *p = &(BinaryNode){
        .kind = node_binary,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
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
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 15,
    };

    IntegerNode *r = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 8,
    };

    BinaryNode *p = &(BinaryNode){
        .kind = node_binary,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
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
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 15,
    };

    IntegerNode *r = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 8,
    };

    BinaryNode *p = &(BinaryNode){
        .kind = node_binary,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
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
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 15,
    };

    IntegerNode *r = &(IntegerNode){
        .kind = node_integer,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
        .value = 8,
    };

    BinaryNode *p = &(BinaryNode){
        .kind = node_binary,
        .line = 1,
        .type = type_get_int32(),
        .is_lvalue = false,
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
        .type = function_type_new(type_get_int32(), NULL, 0, false),
        .params = NULL,
        .num_params = 0,
        .var_args = false,
        .body = NULL,
        .locals = NULL,
        .num_locals = 0,
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
        .type = function_type_new(type_get_void(), NULL, 0, false),
        .params = NULL,
        .num_params = 0,
        .var_args = false,
        .body =
            (StmtNode *)&(CompoundNode){
                .kind = node_compound,
                .line = 1,
                .stmts = NULL,
                .num_stmts = 0,
            },
        .locals = NULL,
        .num_locals = 0,
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

void test_generating_function_with_param(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
        .builder = LLVMCreateBuilder(),
    };

    FunctionNode *p = &(FunctionNode){
        .kind = node_function,
        .line = 1,
        .identifier = "g",
        .type = function_type_new(type_get_void(), (Type *[]){type_get_int32()},
                                  1, false),
        .params =
            (VariableNode *[]){
                &(VariableNode){
                    .kind = node_variable,
                    .line = 1,
                    .identifier = "a",
                    .type = type_get_int32(),
                },
            },
        .num_params = 1,
        .var_args = false,
        .body =
            (StmtNode *)&(CompoundNode){
                .kind = node_compound,
                .line = 1,
                .stmts = NULL,
                .num_stmts = 0,
            },
        .locals = NULL,
        .num_locals = 0,
    };

    LLVMValueRef function = generate_function(ctx, p);
    char *message = LLVMPrintValueToString(function);

    assert(strcmp(message, "\n"
                           "define void @g(i32) {\n"
                           "entry:\n"
                           "  %a = alloca i32\n"
                           "  store i32 %0, i32* %a\n"
                           "  ret void\n"
                           "}\n") == 0);

    LLVMDisposeMessage(message);
    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_function_with_params(void) {
    GeneratorContext *ctx = &(GeneratorContext){
        .module = LLVMModuleCreateWithName("main"),
        .builder = LLVMCreateBuilder(),
    };

    FunctionNode *p = &(FunctionNode){
        .kind = node_function,
        .line = 1,
        .identifier = "g",
        .type = function_type_new(
            type_get_void(), (Type *[]){type_get_int32(), type_get_int32()}, 2,
            false),
        .params =
            (VariableNode *[]){
                &(VariableNode){
                    .kind = node_variable,
                    .line = 1,
                    .identifier = "a",
                    .type = type_get_int32(),
                },
                &(VariableNode){
                    .kind = node_variable,
                    .line = 1,
                    .identifier = "b",
                    .type = type_get_int32(),
                },
            },
        .num_params = 2,
        .var_args = false,
        .body =
            (StmtNode *)&(CompoundNode){
                .kind = node_compound,
                .line = 1,
                .stmts = NULL,
                .num_stmts = 0,
            },
        .locals = NULL,
        .num_locals = 0,
    };

    LLVMValueRef function = generate_function(ctx, p);
    char *message = LLVMPrintValueToString(function);

    assert(strcmp(message, "\n"
                           "define void @g(i32, i32) {\n"
                           "entry:\n"
                           "  %a = alloca i32\n"
                           "  store i32 %0, i32* %a\n"
                           "  %b = alloca i32\n"
                           "  store i32 %1, i32* %b\n"
                           "  ret void\n"
                           "}\n") == 0);

    LLVMDisposeMessage(message);
    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
}

void test_generating_translation_unit(void) {
    TranslationUnitNode *p = parse("test_generating_translation_unit",
                                   "int main(void) {return 42;}\n"
                                   "int add(int x, int y) {return x+y;}",
                                   vec_new());

    LLVMModuleRef module = generate(p);

    char *message = LLVMPrintModuleToString(module);

    assert(strcmp(message,
                  "; ModuleID = 'test_generating_translation_unit'\n"
                  "source_filename = \"test_generating_translation_unit\"\n"
                  "\n"
                  "define i32 @main() {\n"
                  "entry:\n"
                  "  ret i32 42\n"
                  "}\n"
                  "\n"
                  "define i32 @add(i32, i32) {\n"
                  "entry:\n"
                  "  %x = alloca i32\n"
                  "  store i32 %0, i32* %x\n"
                  "  %y = alloca i32\n"
                  "  store i32 %1, i32* %y\n"
                  "  %load = load i32, i32* %x\n"
                  "  %load1 = load i32, i32* %y\n"
                  "  %add = add i32 %load, %load1\n"
                  "  ret i32 %add\n"
                  "}\n") == 0);

    LLVMDisposeMessage(message);
    LLVMDisposeModule(module);
}

void test_generating_call(void) {
    TranslationUnitNode *p = parse("test_generating_call",
                                   "int f(int a);\n"
                                   "int main(void) {return f(42);}\n",
                                   vec_new());

    LLVMModuleRef module = generate(p);

    char *message = LLVMPrintModuleToString(module);

    assert(strcmp(message, "; ModuleID = 'test_generating_call'\n"
                           "source_filename = \"test_generating_call\"\n"
                           "\n"
                           "declare i32 @f(i32)\n"
                           "\n"
                           "define i32 @main() {\n"
                           "entry:\n"
                           "  %call = call i32 @f(i32 42)\n"
                           "  ret i32 %call\n"
                           "}\n") == 0);

    LLVMDisposeMessage(message);
    LLVMDisposeModule(module);
}

void test_generating_if(void) {
    TranslationUnitNode *p =
        parse("test_generating_if", "void f(void) {if (1) {}}\n", vec_new());

    LLVMModuleRef module = generate(p);

    char *message = LLVMPrintModuleToString(module);

    assert(strcmp(message, "; ModuleID = 'test_generating_if'\n"
                           "source_filename = \"test_generating_if\"\n"
                           "\n"
                           "define void @f() {\n"
                           "entry:\n"
                           "  br i1 true, label %then, label %else\n"
                           "\n"
                           "then:                                             "
                           "; preds = %entry\n"
                           "  br label %endif\n"
                           "\n"
                           "else:                                             "
                           "; preds = %entry\n"
                           "  br label %endif\n"
                           "\n"
                           "endif:                                            "
                           "; preds = %else, %then\n"
                           "  ret void\n"
                           "}\n") == 0);

    LLVMDisposeMessage(message);
    LLVMDisposeModule(module);
}

void test_generating_if_else(void) {
    TranslationUnitNode *p =
        parse("test_generating_if_else", "void f(void) {if (1) {} else {}}\n",
              vec_new());

    LLVMModuleRef module = generate(p);

    char *message = LLVMPrintModuleToString(module);

    assert(strcmp(message, "; ModuleID = 'test_generating_if_else'\n"
                           "source_filename = \"test_generating_if_else\"\n"
                           "\n"
                           "define void @f() {\n"
                           "entry:\n"
                           "  br i1 true, label %then, label %else\n"
                           "\n"
                           "then:                                             "
                           "; preds = %entry\n"
                           "  br label %endif\n"
                           "\n"
                           "else:                                             "
                           "; preds = %entry\n"
                           "  br label %endif\n"
                           "\n"
                           "endif:                                            "
                           "; preds = %else, %then\n"
                           "  ret void\n"
                           "}\n") == 0);

    LLVMDisposeMessage(message);
    LLVMDisposeModule(module);
}

void test_generator(void) {
    test_generating_type_void();
    test_generating_type_int32();

    test_generating_integer();
    test_generating_identifier();
    test_generating_negative();
    test_generating_addition();
    test_generating_subtraction();
    test_generating_multiplication();
    test_generating_division();
    test_generating_modulo();

    test_generating_function_prototype();
    test_generating_function();
    test_generating_function_with_param();
    test_generating_function_with_params();

    test_generating_translation_unit();

    test_generating_call();
    test_generating_if();
    test_generating_if_else();
}
