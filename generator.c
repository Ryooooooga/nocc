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

bool generate_compound_stmt(GeneratorContext *ctx, CompoundNode *p) {
    bool is_terminated;
    int i;

    for (i = 0; i < p->stmts->size; i++) {
        is_terminated = generate_stmt(ctx, p->stmts->data[i]);

        if (is_terminated) {
            return true;
        }
    }

    return false;
}

bool generate_return_stmt(GeneratorContext *ctx, ReturnNode *p) {
    LLVMValueRef return_value;

    // TODO: type check
    if (p->return_value) {
        return_value = generate_expr(ctx, p->return_value);

        LLVMBuildRet(ctx->builder, return_value);
    } else {
        LLVMBuildRetVoid(ctx->builder);
    }

    return true;
}

bool generate_expr_stmt(GeneratorContext *ctx, ExprStmtNode *p) {
    generate_expr(ctx, p->expr);

    return false;
}

bool generate_stmt(GeneratorContext *ctx, StmtNode *p) {
    assert(ctx);
    assert(p);

    switch (p->kind) {
    case node_compound:
        return generate_compound_stmt(ctx, (CompoundNode *)p);

    case node_return:
        return generate_return_stmt(ctx, (ReturnNode *)p);

    case node_expr:
        return generate_expr_stmt(ctx, (ExprStmtNode *)p);

    default:
        fprintf(stderr, "unknown statement %d\n", p->kind);
        exit(1);
    }
}

LLVMValueRef generate_function(GeneratorContext *ctx, FunctionNode *p) {
    LLVMTypeRef return_type;
    LLVMTypeRef *param_types;
    LLVMTypeRef function_type;
    LLVMValueRef function;
    LLVMBasicBlockRef entryBasicBlock;
    bool is_terminated;
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

    /* body */
    entryBasicBlock = LLVMAppendBasicBlock(function, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entryBasicBlock);

    is_terminated = generate_stmt(ctx, p->body);

    if (!is_terminated) {
        if (return_type == LLVMVoidType()) {
            LLVMBuildRetVoid(ctx->builder);
        } else if (return_type == LLVMInt32Type()) {
            LLVMBuildRet(
                ctx->builder,
                LLVMConstInt(return_type, (unsigned long long)-1, true));
        } else {
            fprintf(stderr, "unknown return type");
            exit(1);
        }
    }

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
