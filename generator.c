#include "nocc.h"

LLVMBasicBlockRef nearest_break_target(GeneratorContext *ctx) {
    assert(ctx);
    assert(ctx->break_targets->size > 0);

    return vec_back(ctx->break_targets);
}

LLVMBasicBlockRef nearest_continue_target(GeneratorContext *ctx) {
    assert(ctx);
    assert(ctx->continue_targets->size > 0);

    return vec_back(ctx->continue_targets);
}

void push_break_target(GeneratorContext *ctx, LLVMBasicBlockRef target) {
    assert(ctx);
    assert(target);

    vec_push(ctx->break_targets, target);
}

void push_continue_target(GeneratorContext *ctx, LLVMBasicBlockRef target) {
    assert(ctx);
    assert(target);

    vec_push(ctx->continue_targets, target);
}

void pop_break_target(GeneratorContext *ctx) {
    assert(ctx);
    assert(ctx->break_targets->size > 0);

    vec_pop(ctx->break_targets);
}

void pop_continue_target(GeneratorContext *ctx) {
    assert(ctx);
    assert(ctx->continue_targets->size > 0);

    vec_pop(ctx->continue_targets);
}

LLVMTypeRef generate_pointer_type(GeneratorContext *ctx, PointerType *p) {
    LLVMTypeRef element_type;

    element_type = generate_type(ctx, p->element_type);

    return LLVMPointerType(element_type, 0);
}

LLVMTypeRef generate_function_type(GeneratorContext *ctx, FunctionType *p) {
    LLVMTypeRef return_type;
    LLVMTypeRef *param_types;
    int i;

    return_type = generate_type(ctx, p->return_type);
    param_types = malloc(sizeof(LLVMTypeRef) * p->num_params);

    for (i = 0; i < p->num_params; i++) {
        param_types[i] = generate_type(ctx, p->param_types[i]);
    }

    return LLVMFunctionType(return_type, param_types, p->num_params,
                            p->var_args);
}

LLVMTypeRef generate_struct_type(GeneratorContext *ctx, StructType *p) {
    LLVMContextRef context;
    LLVMTypeRef *element_types;
    int i;

    if (p->generated_type) {
        return p->generated_type;
    }

    context = LLVMGetModuleContext(ctx->module);
    p->generated_type = LLVMStructCreateNamed(context, p->identifier);

    if (p->is_incomplete) {
        return p->generated_type;
    }

    element_types = malloc(sizeof(LLVMTypeRef) * p->num_members);

    for (i = 0; i < p->num_members; i++) {
        element_types[i] = generate_type(ctx, p->members[i]->type);
    }

    LLVMStructSetBody(p->generated_type, element_types, p->num_members, false);
    return p->generated_type;
}

LLVMTypeRef generate_type(GeneratorContext *ctx, Type *p) {
    assert(ctx);
    assert(p);

    switch (p->kind) {
    case type_void:
        return LLVMVoidType();

    case type_int8:
        return LLVMInt8Type();

    case type_int32:
        return LLVMInt32Type();

    case type_pointer:
        return generate_pointer_type(ctx, (PointerType *)p);

    case type_function:
        return generate_function_type(ctx, (FunctionType *)p);

    case type_struct:
        return generate_struct_type(ctx, (StructType *)p);

    default:
        fprintf(stderr, "unknown type %d\n", p->kind);
        exit(1);
    }
}

LLVMValueRef generate_integer_expr(GeneratorContext *ctx, IntegerNode *p) {
    (void)ctx;

    return LLVMConstInt(LLVMInt32Type(), p->value, true);
}

LLVMValueRef generate_string_expr(GeneratorContext *ctx, StringNode *p) {
    (void)ctx;

    /* TODO: '\0' in string */
    return LLVMBuildGlobalStringPtr(ctx->builder, p->string, ".str");
}

LLVMValueRef generate_identifier_expr(GeneratorContext *ctx,
                                      IdentifierNode *p) {
    assert(p->declaration->generated_location);

    return LLVMBuildLoad(ctx->builder, p->declaration->generated_location,
                         "load");
}

LLVMValueRef generate_postfix_expr(GeneratorContext *ctx, PostfixNode *p) {
    LLVMValueRef operand;
    LLVMValueRef value;
    LLVMValueRef result;
    LLVMValueRef one;
    LLVMTypeRef type;

    switch (p->operator_) {
    case token_increment:
    case token_decrement:
        operand = generate_expr_addr(ctx, p->operand);
        value = LLVMBuildLoad(ctx->builder, operand, "load");
        type = LLVMTypeOf(value);
        one = LLVMConstInt(type, 1, false);

        if (is_integer_type(p->operand->type)) {
            if (p->operator_ == token_increment) {
                result = LLVMBuildAdd(ctx->builder, value, one, "inc");
            } else {
                result = LLVMBuildSub(ctx->builder, value, one, "dec");
            }
        } else if (is_pointer_type(p->operand->type)) {
            fprintf(stderr, "ptr++ not implemented\n"); /* TODO: increment */
            exit(1);
        } else {
            fprintf(stderr,
                    "unknown postfix operand type of prefix operator ++\n");
            exit(1);
        }

        LLVMBuildStore(ctx->builder, result, operand);
        return value;

    default:
        fprintf(stderr, "unknown unary operator %d\n", p->operator_);
        exit(1);
    }
}

LLVMValueRef generate_call_expr(GeneratorContext *ctx, CallNode *p) {
    LLVMValueRef callee;
    LLVMValueRef *args;
    int i;

    callee = generate_expr(ctx, p->callee);

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
    LLVMValueRef value;
    LLVMValueRef one;
    LLVMTypeRef type;

    switch (p->operator_) {
    case '+':
        operand = generate_expr(ctx, p->operand);
        return operand;

    case '-':
        operand = generate_expr(ctx, p->operand);
        return LLVMBuildNeg(ctx->builder, operand, "neg");

    case '*':
        operand = generate_expr(ctx, p->operand);
        return LLVMBuildLoad(ctx->builder, operand, "deref");

    case '&':
        return generate_expr_addr(ctx, p->operand);

    case token_increment:
    case token_decrement:
        operand = generate_expr_addr(ctx, p->operand);
        value = LLVMBuildLoad(ctx->builder, operand, "load");
        type = LLVMTypeOf(value);
        one = LLVMConstInt(type, 1, false);

        if (is_integer_type(p->operand->type)) {
            if (p->operator_ == token_increment) {
                value = LLVMBuildAdd(ctx->builder, value, one, "inc");
            } else {
                value = LLVMBuildSub(ctx->builder, value, one, "dec");
            }
        } else if (is_pointer_type(p->operand->type)) {
            fprintf(stderr, "++ptr not implemented\n"); /* TODO: increment */
            exit(1);
        } else {
            fprintf(stderr,
                    "unknown unary operand type of prefix operator ++\n");
            exit(1);
        }

        LLVMBuildStore(ctx->builder, value, operand);
        return value;

    default:
        fprintf(stderr, "unknown unary operator %d\n", p->operator_);
        exit(1);
    }
}

LLVMValueRef generate_cast_expr(GeneratorContext *ctx, CastNode *p) {
    LLVMValueRef src;
    LLVMTypeRef dest_type;
    char *src_type_str;
    char *dest_type_str;

    dest_type = generate_type(ctx, p->type);

    switch (p->type->kind) {
    case type_void:
        /* T -> void */
        src = generate_expr(ctx, p->operand);
        return src;

    case type_int8:
        /* T -> int8 */
        src = generate_expr(ctx, p->operand);

        switch (p->operand->type->kind) {
        case type_int8:
            /* int8 -> int8 */
            return src;

        case type_int32:
            /* int32 -> int8 */
            return LLVMBuildTrunc(ctx->builder, src, dest_type, "trunc");

        case type_pointer:
            /* T* -> int8 */
            return LLVMBuildPtrToInt(ctx->builder, src, dest_type, "ptrtoint8");

        default:
            break;
        }
        break;

    case type_int32:
        /* T -> int32 */
        src = generate_expr(ctx, p->operand);

        switch (p->operand->type->kind) {
        case type_int8:
            /* int8 -> int32 */
            return LLVMBuildSExt(ctx->builder, src, dest_type, "sext");

        case type_int32:
            /* int32 -> int32 */
            return src;

        case type_pointer:
            /* T* -> int32 */
            return LLVMBuildPtrToInt(ctx->builder, src, dest_type,
                                     "ptrtoint32");

        default:
            break;
        }
        break;

    case type_pointer:
        /* U -> T* */
        switch (p->operand->type->kind) {
        case type_int8:
        case type_int32:
            /* intN -> T* */
            src = generate_expr(ctx, p->operand);
            return LLVMBuildIntToPtr(ctx->builder, src, dest_type, "inttoptr");

        case type_pointer:
            /* T* -> U* */
            src = generate_expr(ctx, p->operand);
            return LLVMBuildPointerCast(ctx->builder, src, dest_type,
                                        "ptrtoptr");

        case type_array:
            /* [N x T] -> T* */
            src = generate_expr(ctx, p->operand);
            return LLVMBuildInBoundsGEP(ctx->builder, src, NULL, 0, "arrtoptr");

        case type_function:
            /* F -> T* */
            if (type_equals(p->operand->type, pointer_element_type(p->type))) {
                /* F -> F* */
                return generate_expr_addr(ctx, p->operand);
            }
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    src = generate_expr(ctx, p->operand);
    src_type_str = LLVMPrintTypeToString(LLVMTypeOf(src));
    dest_type_str = LLVMPrintTypeToString(dest_type);

    fprintf(stderr, "unimplemented type cast %s -> %s\n", src_type_str,
            dest_type_str);

    LLVMDisposeMessage(dest_type_str);
    LLVMDisposeMessage(src_type_str);
    exit(1);
}

LLVMValueRef generate_binary_expr(GeneratorContext *ctx, BinaryNode *p) {
    LLVMValueRef left;
    LLVMValueRef right;

    LLVMValueRef cmp;

    switch (p->operator_) {
    case '=':
        right = generate_expr(ctx, p->right);
        left = generate_expr_addr(ctx, p->left);

        LLVMBuildStore(ctx->builder, right, left);
        return right;

    default:
        break;
    }

    left = generate_expr(ctx, p->left);
    right = generate_expr(ctx, p->right);

    switch (p->operator_) {
    case '+':
        if (is_pointer_type(p->left->type)) {
            /* T* + int -> T* */
            return LLVMBuildInBoundsGEP(ctx->builder, left, &right, 1,
                                        "ptradd");
        } else if (is_pointer_type(p->right->type)) {
            /* int + T* -> T* */
            return LLVMBuildInBoundsGEP(ctx->builder, right, &left, 1,
                                        "ptradd");
        }

        /* int + int -> int */
        return LLVMBuildAdd(ctx->builder, left, right, "add");

    case '-':
        if (is_pointer_type(p->left->type) && is_pointer_type(p->right->type)) {
            /* T* - T* -> ptrdiff_t */
            left = LLVMBuildPtrDiff(ctx->builder, left, right, "ptrdiff");
            return LLVMBuildTrunc(ctx->builder, left, LLVMInt32Type(),
                                  "ptrdiff_i32");
        } else if (is_pointer_type(p->left->type)) {
            /* T* - int -> T* */
            right = LLVMBuildNeg(ctx->builder, right, "negidx");
            return LLVMBuildInBoundsGEP(ctx->builder, left, &right, 1,
                                        "ptrsub");
        }

        /* int - int -> int */
        return LLVMBuildSub(ctx->builder, left, right, "sub");

    case '*':
        return LLVMBuildMul(ctx->builder, left, right, "mul");

    case '/':
        return LLVMBuildSDiv(ctx->builder, left, right, "div");

    case '%':
        return LLVMBuildSRem(ctx->builder, left, right, "rem");

    case '<':
        switch (p->left->type->kind) {
        case type_int32:
        case type_pointer:
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntSLT, left, right, "cmp");
            return LLVMBuildZExt(ctx->builder, cmp, LLVMInt32Type(), "cmp_i32");

        default:
            fprintf(stderr, "unknown type\n");
            exit(1);
        }

    case '>':
        switch (p->left->type->kind) {
        case type_int32:
        case type_pointer:
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntSGT, left, right, "cmp");
            return LLVMBuildZExt(ctx->builder, cmp, LLVMInt32Type(), "cmp_i32");

        default:
            fprintf(stderr, "unknown type\n");
            exit(1);
        }

    case token_lesser_equal:
        switch (p->left->type->kind) {
        case type_int32:
        case type_pointer:
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntSLE, left, right, "cmp");
            return LLVMBuildZExt(ctx->builder, cmp, LLVMInt32Type(), "cmp_i32");

        default:
            fprintf(stderr, "unknown type\n");
            exit(1);
        }

    case token_greater_equal:
        switch (p->left->type->kind) {
        case type_int32:
        case type_pointer:
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntSGE, left, right, "cmp");
            return LLVMBuildZExt(ctx->builder, cmp, LLVMInt32Type(), "cmp_i32");

        default:
            fprintf(stderr, "unknown type\n");
            exit(1);
        }

    case token_equal:
        switch (p->left->type->kind) {
        case type_int32:
        case type_pointer:
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntEQ, left, right, "cmp");
            return LLVMBuildZExt(ctx->builder, cmp, LLVMInt32Type(), "cmp_i32");

        default:
            fprintf(stderr, "unknown type\n");
            exit(1);
        }

    case token_not_equal:
        switch (p->left->type->kind) {
        case type_int32:
        case type_pointer:
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntNE, left, right, "cmp");
            return LLVMBuildZExt(ctx->builder, cmp, LLVMInt32Type(), "cmp_i32");

        default:
            fprintf(stderr, "unknown type\n");
            exit(1);
        }

    default:
        fprintf(stderr, "unknown binary operator %d\n", p->operator_);
        exit(1);
    }
}

LLVMValueRef generate_dot_expr(GeneratorContext *ctx, DotNode *p) {
    LLVMValueRef parent;

    parent = generate_expr(ctx, p->parent);

    return LLVMBuildExtractValue(ctx->builder, parent, p->index, "member");
}

LLVMValueRef generate_expr(GeneratorContext *ctx, ExprNode *p) {
    assert(p);
    assert(ctx);

    switch (p->kind) {
    case node_integer:
        return generate_integer_expr(ctx, (IntegerNode *)p);

    case node_string:
        return generate_string_expr(ctx, (StringNode *)p);

    case node_identifier:
        return generate_identifier_expr(ctx, (IdentifierNode *)p);

    case node_postfix:
        return generate_postfix_expr(ctx, (PostfixNode *)p);

    case node_call:
        return generate_call_expr(ctx, (CallNode *)p);

    case node_unary:
        return generate_unary_expr(ctx, (UnaryNode *)p);

    case node_cast:
        return generate_cast_expr(ctx, (CastNode *)p);

    case node_binary:
        return generate_binary_expr(ctx, (BinaryNode *)p);

    case node_dot:
        return generate_dot_expr(ctx, (DotNode *)p);

    default:
        fprintf(stderr, "unknown expression %d\n", p->kind);
        exit(1);
    }
}

LLVMValueRef generate_identifier_expr_addr(GeneratorContext *ctx,
                                           IdentifierNode *p) {
    assert(p->declaration->generated_location);

    (void)ctx;

    return p->declaration->generated_location;
}

LLVMValueRef generate_unary_expr_addr(GeneratorContext *ctx, UnaryNode *p) {
    assert(p->is_lvalue);

    switch (p->operator_) {
    case '*':
        return generate_expr(ctx, p->operand);

    default:
        fprintf(stderr, "unknown unary expression address %d\n", p->operator_);
        exit(1);
    }
}

LLVMValueRef generate_dot_expr_addr(GeneratorContext *ctx, DotNode *p) {
    LLVMValueRef parent;

    assert(p->is_lvalue);

    parent = generate_expr_addr(ctx, p->parent);

    return LLVMBuildStructGEP(ctx->builder, parent, p->index, "memaddr");
}

LLVMValueRef generate_expr_addr(GeneratorContext *ctx, ExprNode *p) {
    assert(p);
    assert(ctx);

    if (!p->is_lvalue) {
        fprintf(stderr, "expression must be a lvalue %d\n", p->kind);
        exit(1);
    }

    switch (p->kind) {
    case node_identifier:
        return generate_identifier_expr_addr(ctx, (IdentifierNode *)p);

    case node_unary:
        return generate_unary_expr_addr(ctx, (UnaryNode *)p);

    case node_dot:
        return generate_dot_expr_addr(ctx, (DotNode *)p);

    default:
        fprintf(stderr, "unknown expression address %d\n", p->kind);
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

bool generate_while_stmt(GeneratorContext *ctx, WhileNode *p) {
    LLVMValueRef function;
    LLVMBasicBlockRef condition_basic_block;
    LLVMBasicBlockRef body_basic_block;
    LLVMBasicBlockRef endwhile_basic_block;

    LLVMValueRef condition;
    LLVMValueRef bool_condition;

    function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    condition_basic_block = LLVMAppendBasicBlock(function, "cond");
    body_basic_block = LLVMAppendBasicBlock(function, "body");
    endwhile_basic_block = LLVMAppendBasicBlock(function, "endwhile");

    LLVMBuildBr(ctx->builder, condition_basic_block);

    /* condition */
    LLVMPositionBuilderAtEnd(ctx->builder, condition_basic_block);

    condition = generate_expr(ctx, p->condition);
    bool_condition =
        LLVMBuildICmp(ctx->builder, LLVMIntNE, condition,
                      LLVMConstInt(LLVMTypeOf(condition), 0, true), "cond");

    LLVMBuildCondBr(ctx->builder, bool_condition, body_basic_block,
                    endwhile_basic_block);

    /* body */
    LLVMPositionBuilderAtEnd(ctx->builder, body_basic_block);

    push_break_target(ctx, endwhile_basic_block);
    push_continue_target(ctx, condition_basic_block);

    if (!generate_stmt(ctx, p->body)) {
        LLVMBuildBr(ctx->builder, condition_basic_block);
    }

    pop_break_target(ctx);
    pop_continue_target(ctx);

    /* end while */
    LLVMPositionBuilderAtEnd(ctx->builder, endwhile_basic_block);

    return false;
}

bool generate_do_stmt(GeneratorContext *ctx, DoNode *p) {
    LLVMValueRef function;
    LLVMBasicBlockRef body_basic_block;
    LLVMBasicBlockRef condition_basic_block;
    LLVMBasicBlockRef enddo_basic_block;

    LLVMValueRef condition;
    LLVMValueRef bool_condition;

    function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    body_basic_block = LLVMAppendBasicBlock(function, "body");
    condition_basic_block = LLVMAppendBasicBlock(function, "cond");
    enddo_basic_block = LLVMAppendBasicBlock(function, "enddo");

    LLVMBuildBr(ctx->builder, body_basic_block);

    /* body */
    LLVMPositionBuilderAtEnd(ctx->builder, body_basic_block);

    push_break_target(ctx, enddo_basic_block);
    push_continue_target(ctx, condition_basic_block);

    if (!generate_stmt(ctx, p->body)) {
        LLVMBuildBr(ctx->builder, condition_basic_block);
    }

    pop_break_target(ctx);
    pop_continue_target(ctx);

    /* condition */
    LLVMPositionBuilderAtEnd(ctx->builder, condition_basic_block);

    condition = generate_expr(ctx, p->condition);
    bool_condition =
        LLVMBuildICmp(ctx->builder, LLVMIntNE, condition,
                      LLVMConstInt(LLVMTypeOf(condition), 0, true), "cond");

    LLVMBuildCondBr(ctx->builder, bool_condition, body_basic_block,
                    enddo_basic_block);

    /* end do */
    LLVMPositionBuilderAtEnd(ctx->builder, enddo_basic_block);

    return false;
}

bool generate_for_stmt(GeneratorContext *ctx, ForNode *p) {
    LLVMValueRef function;
    LLVMBasicBlockRef condition_basic_block;
    LLVMBasicBlockRef body_basic_block;
    LLVMBasicBlockRef continuation_basic_block;
    LLVMBasicBlockRef endfor_basic_block;

    LLVMValueRef condition;
    LLVMValueRef bool_condition;

    function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    condition_basic_block = LLVMAppendBasicBlock(function, "cond");
    body_basic_block = LLVMAppendBasicBlock(function, "body");
    continuation_basic_block = LLVMAppendBasicBlock(function, "cont");
    endfor_basic_block = LLVMAppendBasicBlock(function, "endfor");

    /* initialization */
    if (p->initialization) {
        generate_expr(ctx, p->initialization);
    }

    LLVMBuildBr(ctx->builder, condition_basic_block);

    /* condition */
    LLVMPositionBuilderAtEnd(ctx->builder, condition_basic_block);

    if (p->condition) {
        condition = generate_expr(ctx, p->condition);
        bool_condition =
            LLVMBuildICmp(ctx->builder, LLVMIntNE, condition,
                          LLVMConstInt(LLVMTypeOf(condition), 0, true), "cond");

        LLVMBuildCondBr(ctx->builder, bool_condition, body_basic_block,
                        endfor_basic_block);
    } else {
        LLVMBuildBr(ctx->builder, body_basic_block);
    }

    /* body */
    LLVMPositionBuilderAtEnd(ctx->builder, body_basic_block);

    push_break_target(ctx, endfor_basic_block);
    push_continue_target(ctx, continuation_basic_block);

    if (!generate_stmt(ctx, p->body)) {
        LLVMBuildBr(ctx->builder, continuation_basic_block);
    }

    pop_break_target(ctx);
    pop_continue_target(ctx);

    /* continuation */
    LLVMPositionBuilderAtEnd(ctx->builder, continuation_basic_block);

    if (p->continuation) {
        generate_expr(ctx, p->continuation);
    }

    LLVMBuildBr(ctx->builder, condition_basic_block);

    /* end for */
    LLVMPositionBuilderAtEnd(ctx->builder, endfor_basic_block);

    return false;
}

bool generate_break_stmt(GeneratorContext *ctx, BreakNode *p) {
    (void)p;

    LLVMBuildBr(ctx->builder, nearest_break_target(ctx));

    return true;
}

bool generate_continue_stmt(GeneratorContext *ctx, ContinueNode *p) {
    (void)p;

    LLVMBuildBr(ctx->builder, nearest_continue_target(ctx));

    return true;
}

bool generate_decl_stmt(GeneratorContext *ctx, DeclStmtNode *p) {
    (void)ctx;
    (void)p;

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

    case node_while:
        return generate_while_stmt(ctx, (WhileNode *)p);

    case node_do:
        return generate_do_stmt(ctx, (DoNode *)p);

    case node_for:
        return generate_for_stmt(ctx, (ForNode *)p);

    case node_break:
        return generate_break_stmt(ctx, (BreakNode *)p);

    case node_continue:
        return generate_continue_stmt(ctx, (ContinueNode *)p);

    case node_decl:
        return generate_decl_stmt(ctx, (DeclStmtNode *)p);

    case node_expr:
        return generate_expr_stmt(ctx, (ExprStmtNode *)p);

    default:
        fprintf(stderr, "unknown statement %d\n", p->kind);
        exit(1);
    }
}

void generate_global_variable(GeneratorContext *ctx, VariableNode *p) {
    LLVMTypeRef type;

    type = generate_type(ctx, p->type);

    p->generated_location = LLVMAddGlobal(ctx->module, type, p->identifier);

    switch (p->type->kind) {
    case type_int8:
    case type_int32:
        LLVMSetInitializer(p->generated_location, LLVMConstInt(type, 0, true));
        break;

    case type_pointer:
        LLVMSetInitializer(p->generated_location, LLVMConstPointerNull(type));
        break;

    default:
        fprintf(stderr, "unknown type of global variable %s\n", p->identifier);
        exit(1);
    }
}

LLVMValueRef generate_function(GeneratorContext *ctx, FunctionNode *p) {
    LLVMTypeRef func_type;
    LLVMTypeRef return_type;
    LLVMTypeRef *param_types;
    LLVMValueRef function;
    LLVMBasicBlockRef entry_basic_block;

    bool is_terminated;
    int i;

    assert(ctx);
    assert(p);
    assert(is_function_type(p->type));

    /* build LLVM function type */
    func_type = generate_type(ctx, p->type);
    return_type = LLVMGetReturnType(func_type);
    param_types = malloc(sizeof(LLVMTypeRef) * LLVMCountParamTypes(func_type));
    LLVMGetParamTypes(func_type, param_types);

    /* find function */
    function = LLVMGetNamedFunction(ctx->module, p->identifier);

    if (function == NULL) {
        /* create function if not found */
        function = LLVMAddFunction(ctx->module, p->identifier, func_type);
    }

    if (LLVMGetElementType(LLVMTypeOf(function)) != func_type) {
        fprintf(stderr, "type mismatch of function %s\n", p->identifier);
        exit(1);
    }

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

    for (i = 0; i < p->num_locals; i++) {
        /* allocate local location */
        p->locals[i]->generated_location = LLVMBuildAlloca(
            ctx->builder, generate_type(ctx, p->locals[i]->type),
            p->locals[i]->identifier);
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
    case node_typedef:
        return;

    case node_variable:
        generate_global_variable(ctx, (VariableNode *)p);
        return;

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
    ctx.break_targets = vec_new();
    ctx.continue_targets = vec_new();

    for (i = 0; i < p->num_decls; i++) {
        generate_decl(&ctx, p->decls[i]);
    }

    LLVMDisposeBuilder(ctx.builder);
    LLVMVerifyModule(ctx.module, LLVMAbortProcessAction, NULL);

    return ctx.module;
}
