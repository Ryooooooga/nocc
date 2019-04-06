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

LLVMValueRef generate_identifier_expr(GeneratorContext *ctx,
                                      IdentifierNode *p) {
    return LLVMBuildLoad(ctx->builder, p->declaration->generated_location,
                         "load");
}

LLVMValueRef generate_call_expr(GeneratorContext *ctx, CallNode *p) {
    LLVMValueRef callee;
    LLVMValueRef *args;
    int i;

    callee = generate_expr_addr(ctx, p->callee);

    args = malloc(sizeof(LLVMValueRef) * p->num_args);

    for (i = 0; i < p->num_args; i++) {
        args[i] = generate_expr(ctx, p->args[i]);
    }

    if (LLVMGetReturnType(LLVMGetElementType(LLVMTypeOf(callee))) ==
        LLVMVoidType()) {
        return LLVMBuildCall(ctx->builder, callee, args, p->num_args, "");
    } else {
        return LLVMBuildCall(ctx->builder, callee, args, p->num_args, "call");
    }
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

    LLVMValueRef cmp;

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

    case '<':
        cmp = LLVMBuildICmp(ctx->builder, LLVMIntSLT, left, right, "lt");
        return LLVMBuildSExt(ctx->builder, cmp, LLVMInt32Type(), "lt_i32");

    case '>':
        cmp = LLVMBuildICmp(ctx->builder, LLVMIntSGT, left, right, "gt");
        return LLVMBuildSExt(ctx->builder, cmp, LLVMInt32Type(), "gt_i32");

    case token_lesser_equal:
        cmp = LLVMBuildICmp(ctx->builder, LLVMIntSLE, left, right, "le");
        return LLVMBuildSExt(ctx->builder, cmp, LLVMInt32Type(), "le_i32");

    case token_greater_equal:
        cmp = LLVMBuildICmp(ctx->builder, LLVMIntSGE, left, right, "ge");
        return LLVMBuildSExt(ctx->builder, cmp, LLVMInt32Type(), "ge_i32");

    case token_equal:
        cmp = LLVMBuildICmp(ctx->builder, LLVMIntEQ, left, right, "eq");
        return LLVMBuildSExt(ctx->builder, cmp, LLVMInt32Type(), "eq_i32");

    case token_not_equal:
        cmp = LLVMBuildICmp(ctx->builder, LLVMIntNE, left, right, "ne");
        return LLVMBuildSExt(ctx->builder, cmp, LLVMInt32Type(), "ne_i32");

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

    case node_identifier:
        return generate_identifier_expr(ctx, (IdentifierNode *)p);

    case node_call:
        return generate_call_expr(ctx, (CallNode *)p);

    case node_unary:
        return generate_unary_expr(ctx, (UnaryNode *)p);

    case node_binary:
        return generate_binary_expr(ctx, (BinaryNode *)p);

    default:
        fprintf(stderr, "unknown expression %d\n", p->kind);
        exit(1);
    }
}

LLVMValueRef generate_identifier_expr_addr(GeneratorContext *ctx,
                                           IdentifierNode *p) {
    (void)ctx;

    return p->declaration->generated_location;
}

LLVMValueRef generate_expr_addr(GeneratorContext *ctx, ExprNode *p) {
    assert(p);
    assert(ctx);

    switch (p->kind) {
    case node_identifier:
        return generate_identifier_expr_addr(ctx, (IdentifierNode *)p);

    default:
        fprintf(stderr, "unknown expression %d\n", p->kind);
        exit(1);
    }
}

bool generate_compound_stmt(GeneratorContext *ctx, CompoundNode *p) {
    bool is_terminated;
    int i;

    for (i = 0; i < p->num_stmts; i++) {
        is_terminated = generate_stmt(ctx, p->stmts[i]);

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

bool generate_if_stmt(GeneratorContext *ctx, IfNode *p) {
    LLVMValueRef function;
    LLVMBasicBlockRef then_basic_block;
    LLVMBasicBlockRef else_basic_block;
    LLVMBasicBlockRef endif_basic_block;

    LLVMValueRef condition;
    LLVMValueRef bool_condition;

    function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    then_basic_block = LLVMAppendBasicBlock(function, "then");
    else_basic_block = LLVMAppendBasicBlock(function, "else");
    endif_basic_block = LLVMAppendBasicBlock(function, "endif");

    /* condition */
    condition = generate_expr(ctx, p->condition);
    bool_condition =
        LLVMBuildICmp(ctx->builder, LLVMIntNE, condition,
                      LLVMConstInt(LLVMTypeOf(condition), 0, true), "cond");

    LLVMBuildCondBr(ctx->builder, bool_condition, then_basic_block,
                    else_basic_block);

    /* then */
    LLVMPositionBuilderAtEnd(ctx->builder, then_basic_block);

    if (!generate_stmt(ctx, p->then)) {
        LLVMBuildBr(ctx->builder, endif_basic_block);
    }

    /* else */
    LLVMPositionBuilderAtEnd(ctx->builder, else_basic_block);

    if (p->else_ == NULL || !generate_stmt(ctx, p->else_)) {
        LLVMBuildBr(ctx->builder, endif_basic_block);
    }

    /* end if */
    LLVMPositionBuilderAtEnd(ctx->builder, endif_basic_block);

    return false;
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

    case node_if:
        return generate_if_stmt(ctx, (IfNode *)p);

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
    LLVMBasicBlockRef entry_basic_block;

    FunctionType *type;
    bool is_terminated;
    int i;

    assert(ctx);
    assert(p);
    assert(p->type->kind == type_function);

    type = (FunctionType *)p->type;
    return_type = generate_type(ctx, type->return_type);

    param_types = malloc(sizeof(LLVMTypeRef) * type->num_params);

    for (i = 0; i < type->num_params; i++) {
        param_types[i] = generate_type(ctx, type->param_types[i]);
    }

    function_type = LLVMFunctionType(return_type, param_types, type->num_params,
                                     p->var_args);

    function = LLVMAddFunction(ctx->module, p->identifier, function_type);

    p->generated_location = function;

    if (p->body == NULL) {
        return function;
    }

    /* entry block */
    entry_basic_block = LLVMAppendBasicBlock(function, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry_basic_block);

    /* prologue */
    for (i = 0; i < p->num_params; i++) {
        /* allocate parameter location */
        p->params[i]->generated_location = LLVMBuildAlloca(
            ctx->builder, param_types[i], p->params[i]->identifier);

        /* store parameter */
        LLVMBuildStore(ctx->builder, LLVMGetParam(function, i),
                       p->params[i]->generated_location);
    }

    /* body */
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

LLVMModuleRef generate(TranslationUnitNode *p) {
    GeneratorContext ctx;
    int i;

    assert(p);

    ctx.module = LLVMModuleCreateWithName(p->filename);
    ctx.builder = LLVMCreateBuilder();

    for (i = 0; i < p->num_decls; i++) {
        generate_decl(&ctx, p->decls[i]);
    }

    LLVMDisposeBuilder(ctx.builder);
    LLVMVerifyModule(ctx.module, LLVMAbortProcessAction, NULL);

    return ctx.module;
}
