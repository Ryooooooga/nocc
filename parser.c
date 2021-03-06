#include "nocc.h"

const Token *current_token(ParserContext *ctx) {
    assert(ctx != NULL);
    return ctx->tokens[ctx->index];
}

const Token *peek_token(ParserContext *ctx) {
    assert(ctx != NULL);

    if (current_token(ctx)->kind == '\0') {
        return current_token(ctx);
    }

    return ctx->tokens[ctx->index + 1];
}

const Token *consume_token(ParserContext *ctx) {
    assert(ctx != NULL);

    if (current_token(ctx)->kind == '\0') {
        return current_token(ctx);
    }

    return ctx->tokens[ctx->index++];
}

const Token *consume_token_if(ParserContext *ctx, int kind) {
    assert(ctx != NULL);

    if (current_token(ctx)->kind == kind) {
        return consume_token(ctx);
    }

    return NULL;
}

const Token *expect_token(ParserContext *ctx, int expected_token_kind) {
    assert(ctx != NULL);

    if (current_token(ctx)->kind == expected_token_kind) {
        return consume_token(ctx);
    }

    fprintf(stderr, "error at line %d: expected %d, but got %s\n",
            current_token(ctx)->line, expected_token_kind,
            current_token(ctx)->text); /* TODO: better error message */
    exit(1);
}

bool is_type_specifier_token(ParserContext *ctx, const Token *t) {
    DeclNode *symbol;

    assert(ctx != NULL);
    assert(t != NULL);

    switch (t->kind) {
    case token_void:
    case token_char:
    case token_int:
    case token_long:
    case token_unsigned:
    case token_struct:
    case token_const:
        return true;

    case token_identifier:
        symbol = scope_stack_find(ctx->env, t->text, true);

        return symbol && symbol->kind == node_typedef;

    default:
        return false;
    }
}

bool is_declaration_specifier_token(ParserContext *ctx, const Token *t) {
    switch (t->kind) {
    case token_typedef:
    case token_extern:
        return true;

    default:
        return is_type_specifier_token(ctx, t);
    }
}

MemberNode *parse_struct_member(ParserContext *ctx) {
    Type *type;
    const Token *t;

    /* type */
    type = parse_type(ctx);

    /* declarator */
    parse_declarator(ctx, &type, &t);

    /* ; */
    expect_token(ctx, ';');

    /* make node */
    return sema_struct_member(ctx, type, t);
}

Type *parse_identifier_type(ParserContext *ctx) {
    const Token *t;

    /* identifier */
    t = expect_token(ctx, token_identifier);

    /* get type */
    return sema_identifier_type(ctx, t);
}

Type *parse_struct_type(ParserContext *ctx) {
    StructType *type;

    const Token *t;
    const Token *identifier;
    Vec *members;

    /* struct */
    t = expect_token(ctx, token_struct);

    /* identifier */
    identifier = expect_token(ctx, token_identifier);

    /* {? */
    if (consume_token_if(ctx, '{') == NULL) {
        return sema_struct_type_without_body(ctx, t, identifier);
    }

    /* register symbol and enter struct scope */
    type = sema_struct_type_enter(ctx, t, identifier);

    /* member declarations */
    members = vec_new();

    while (current_token(ctx)->kind != '}') {
        vec_push(members, parse_struct_member(ctx));
    }

    /* } */
    expect_token(ctx, '}');

    /* leave struct scope and make node */
    return sema_struct_type_leave(ctx, type, t, (MemberNode **)members->data,
                                  members->size);
}

Type *parse_primary_type(ParserContext *ctx) {
    switch (current_token(ctx)->kind) {
    case token_void:
        expect_token(ctx, token_void);
        return type_get_void();

    case token_char:
        expect_token(ctx, token_char);
        return type_get_int8();

    case token_unsigned:
        expect_token(ctx, token_unsigned);
        return parse_primary_type(ctx); /* TODO: unsigned type */

    case token_int:
    case token_long:        /* TODO: long type */
        consume_token(ctx); /* eat int or long */
        return type_get_int32();

    case token_identifier:
        return parse_identifier_type(ctx);

    case token_struct:
        return parse_struct_type(ctx);

    default:
        fprintf(stderr, "error at line %d: expected type, but got %s\n",
                current_token(ctx)->line, current_token(ctx)->text);
        exit(1);
    }
}

Type *parse_type(ParserContext *ctx) {
    bool is_const;
    Type *type;

    assert(ctx != NULL);

    /* const? */
    is_const = consume_token_if(ctx, token_const) != NULL;

    /* primary type */
    type = parse_primary_type(ctx);

    /* TODO: const type */
    (void)is_const;

    return type;
}

ExprNode *parse_paren_expr(ParserContext *ctx) {
    const Token *open;
    const Token *close;
    ExprNode *expr;

    /* ( */
    open = expect_token(ctx, '(');

    /* expression */
    expr = parse_expr(ctx);

    /* ) */
    close = expect_token(ctx, ')');

    /* make node */
    return sema_paren_expr(ctx, open, expr, close);
}

ExprNode *parse_number_expr(ParserContext *ctx) {
    const Token *t;
    long value;

    /* number */
    t = expect_token(ctx, token_number);

    /* convert */
    errno = 0;
    value = strtol(t->text, NULL, 10);

    if (errno == ERANGE || value > INT_MAX) {
        fprintf(stderr, "error at line %d: too large integer constant %s\n",
                current_token(ctx)->line, t->text);
        exit(1);
    }

    /* make node */
    return sema_integer_expr(ctx, t, (int)value);
}

ExprNode *parse_character_expr(ParserContext *ctx) {
    const Token *t;

    /* character */
    t = expect_token(ctx, token_character);

    /* make node */
    return sema_integer_expr(ctx, t, t->string[0]);
}

ExprNode *parse_string_expr(ParserContext *ctx) {
    const Token *t;

    /* string */
    t = expect_token(ctx, token_string);

    /* make node */
    return sema_string_expr(ctx, t, t->string, t->len_string);
}

ExprNode *parse_identifier_expr(ParserContext *ctx) {
    const Token *t;

    /* identifier */
    t = expect_token(ctx, token_identifier);

    /* make node */
    return sema_identifier_expr(ctx, t);
}

ExprNode *parse_primary_expr(ParserContext *ctx) {
    switch (current_token(ctx)->kind) {
    case '(':
        return parse_paren_expr(ctx);

    case token_number:
        return parse_number_expr(ctx);

    case token_character:
        return parse_character_expr(ctx);

    case token_string:
        return parse_string_expr(ctx);

    case token_identifier:
        return parse_identifier_expr(ctx);

    default:
        fprintf(stderr, "error at line %d: expected expression, but got %s\n",
                current_token(ctx)->line, current_token(ctx)->text);
        exit(1);
    }
}

ExprNode *parse_call_expr(ParserContext *ctx, ExprNode *callee) {
    const Token *open;
    const Token *close;
    Vec *args;

    /* ( */
    open = expect_token(ctx, '(');

    /* argument list */
    args = vec_new();

    if (current_token(ctx)->kind != ')') {
        /* expression */
        vec_push(args, parse_assign_expr(ctx));

        /* {, expression} */
        while (consume_token_if(ctx, ',') != NULL) {
            /* expression */
            vec_push(args, parse_assign_expr(ctx));
        }
    }

    /* ) */
    close = expect_token(ctx, ')');

    /* make node */
    return sema_call_expr(ctx, callee, open, (ExprNode **)args->data,
                          args->size, close);
}

ExprNode *parse_index_expr(ParserContext *ctx, ExprNode *operand) {
    const Token *t;
    ExprNode *index;

    /* [ */
    t = expect_token(ctx, '[');

    /* expression */
    index = parse_expr(ctx);

    /* ] */
    expect_token(ctx, ']');

    return sema_binary_expr(ctx, operand, t, index);
}

ExprNode *parse_dot_expr(ParserContext *ctx, ExprNode *parent) {
    const Token *t;
    const Token *identifier;

    /* . */
    t = expect_token(ctx, '.');

    /* identifier */
    identifier = expect_token(ctx, token_identifier);

    /* make node */
    return sema_dot_expr(ctx, parent, t, identifier);
}

ExprNode *parse_arrow_expr(ParserContext *ctx, ExprNode *parent) {
    const Token *t;
    const Token *identifier;

    /* -> */
    t = expect_token(ctx, token_arrow);

    /* identifier */
    identifier = expect_token(ctx, token_identifier);

    /* make node */
    return sema_arrow_expr(ctx, parent, t, identifier);
}

ExprNode *parse_postfix_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *operand;

    operand = parse_primary_expr(ctx);

    while (1) {
        switch (current_token(ctx)->kind) {
        case token_increment:
        case token_decrement:
            t = consume_token(ctx); /* eat postfix operator */
            operand = sema_postfix_expr(ctx, operand, t);
            break;

        case '(':
            operand = parse_call_expr(ctx, operand);
            break;

        case '[':
            operand = parse_index_expr(ctx, operand);
            break;

        case '.':
            operand = parse_dot_expr(ctx, operand);
            break;

        case token_arrow:
            operand = parse_arrow_expr(ctx, operand);
            break;

        default:
            return operand;
        }
    }
}

ExprNode *parse_cast_expr(ParserContext *ctx) {
    const Token *open;
    const Token *close;
    Type *type;
    ExprNode *operand;

    /* ( */
    open = expect_token(ctx, '(');

    /* type */
    type = parse_type(ctx);

    /* abstract declarator */
    parse_abstract_declarator(ctx, &type);

    /* ) */
    close = expect_token(ctx, ')');

    /* unary expression */
    operand = parse_unary_expr(ctx);

    /* make node */
    return sema_cast_expr(ctx, open, type, close, operand);
}

ExprNode *parse_sizeof_expr(ParserContext *ctx) {
    const Token *t;
    Type *type;

    /* sizeof */
    t = expect_token(ctx, token_sizeof);

    if (current_token(ctx)->kind == '(' &&
        is_type_specifier_token(ctx, peek_token(ctx))) {
        /* sizeof ( type ) */
        /* ( */
        expect_token(ctx, '(');

        /* type */
        type = parse_type(ctx);

        /* abstract declarator */
        parse_abstract_declarator(ctx, &type);

        /* ) */
        expect_token(ctx, ')');

        /* make node */
        return sema_sizeof_expr(ctx, t, type);
    }

    /* sizeof unary_expr */
    /* unary expression */
    type = parse_unary_expr(ctx)->type;

    /* make node */
    return sema_sizeof_expr(ctx, t, type);
}

ExprNode *parse_unary_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *operand;

    assert(ctx != NULL);

    t = current_token(ctx);

    switch (t->kind) {
    case '+':
    case '-':
    case '*':
    case '&':
    case '!':
    case token_increment:
    case token_decrement:
        /* unary operator */
        consume_token(ctx);

        /* unary expression */
        operand = parse_unary_expr(ctx);

        /* make node */
        return sema_unary_expr(ctx, t, operand);

    case token_sizeof:
        return parse_sizeof_expr(ctx);

    case '(':
        if (is_type_specifier_token(ctx, peek_token(ctx))) {
            return parse_cast_expr(ctx);
        }
        break;

    default:
        break;
    }

    return parse_postfix_expr(ctx);
}

ExprNode *parse_multiplicative_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* unary expression */
    left = parse_unary_expr(ctx);

    while (current_token(ctx)->kind == '*' || current_token(ctx)->kind == '/' ||
           current_token(ctx)->kind == '%') {
        /* multiplicative operator */
        t = consume_token(ctx);

        /* unary expression */
        right = parse_unary_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_additive_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* multiplicative expression */
    left = parse_multiplicative_expr(ctx);

    while (current_token(ctx)->kind == '+' || current_token(ctx)->kind == '-') {
        /* additive operator */
        t = consume_token(ctx);

        /* multiplicative expression */
        right = parse_multiplicative_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_relational_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* additive expression */
    left = parse_additive_expr(ctx);

    while (current_token(ctx)->kind == '<' || current_token(ctx)->kind == '>' ||
           current_token(ctx)->kind == token_lesser_equal ||
           current_token(ctx)->kind == token_greater_equal) {
        /* relational operator */
        t = consume_token(ctx);

        /* additive expression */
        right = parse_additive_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_equality_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* relational expression */
    left = parse_relational_expr(ctx);

    while (current_token(ctx)->kind == token_equal ||
           current_token(ctx)->kind == token_not_equal) {
        /* equality operator */
        t = consume_token(ctx);

        /* relational expression */
        right = parse_relational_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_bitwise_and_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* equality expression */
    left = parse_equality_expr(ctx);

    while (current_token(ctx)->kind == '&') {
        /* & */
        t = expect_token(ctx, '&');

        /* equality expression */
        right = parse_equality_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_bitwise_xor_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* bitwise and expression */
    left = parse_bitwise_and_expr(ctx);

    while (current_token(ctx)->kind == '^') {
        /* ^ */
        t = expect_token(ctx, '^');

        /* bitwise and expression */
        right = parse_bitwise_and_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_bitwise_or_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* bitwise xor expression */
    left = parse_bitwise_xor_expr(ctx);

    while (current_token(ctx)->kind == '|') {
        /* | */
        t = expect_token(ctx, '|');

        /* bitwise xor expression */
        right = parse_bitwise_xor_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_logical_and_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* bitwise or expression */
    left = parse_bitwise_or_expr(ctx);

    while (current_token(ctx)->kind == token_and) {
        /* && */
        t = expect_token(ctx, token_and);

        /* bitwise or expression */
        right = parse_bitwise_or_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_logical_or_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    /* logical and expression */
    left = parse_logical_and_expr(ctx);

    while (current_token(ctx)->kind == token_or) {
        /* || */
        t = expect_token(ctx, token_or);

        /* logical and expression */
        right = parse_logical_and_expr(ctx);

        /* make node */
        left = sema_binary_expr(ctx, left, t, right);
    }

    return left;
}

ExprNode *parse_assign_expr(ParserContext *ctx) {
    const Token *t;
    ExprNode *left;
    ExprNode *right;

    assert(ctx != NULL);

    /* logical or expression */
    left = parse_logical_or_expr(ctx);

    /* assignment operator */
    if (current_token(ctx)->kind != '=') {
        return left;
    }

    t = expect_token(ctx, '=');

    /* assignment expression */
    right = parse_assign_expr(ctx);

    /* make node */
    return sema_binary_expr(ctx, left, t, right);
}

ExprNode *parse_expr(ParserContext *ctx) {
    assert(ctx != NULL);

    return parse_assign_expr(ctx);
}

StmtNode *parse_compound_stmt(ParserContext *ctx) {
    const Token *open;
    const Token *close;
    Vec *stmts;

    /* enter scope */
    sema_compound_stmt_enter(ctx);

    /* { */
    open = expect_token(ctx, '{');

    /* {statement} */
    stmts = vec_new();

    while (current_token(ctx)->kind != '}') {
        vec_push(stmts, parse_stmt(ctx));
    }

    /* } */
    close = expect_token(ctx, '}');

    /* make node and leave scope */
    return sema_compound_stmt_leave(ctx, open, (StmtNode **)stmts->data,
                                    stmts->size, close);
}

StmtNode *parse_return_stmt(ParserContext *ctx) {
    const Token *t;
    ExprNode *return_value;

    /* return */
    t = expect_token(ctx, token_return);

    /* ;? */
    if (consume_token_if(ctx, ';') != NULL) {
        return sema_return_stmt(ctx, t, NULL);
    }

    /* expression */
    return_value = parse_expr(ctx);

    /* ; */
    expect_token(ctx, ';');

    /* make node */
    return sema_return_stmt(ctx, t, return_value);
}

StmtNode *parse_if_stmt(ParserContext *ctx) {
    const Token *t;
    ExprNode *condition;
    StmtNode *then;
    StmtNode *else_;

    /* if */
    t = expect_token(ctx, token_if);

    /* ( expression ) */
    condition = parse_paren_expr(ctx);

    /* enter then scope */
    sema_if_stmt_enter_block(ctx);

    /* statement */
    then = parse_stmt(ctx);

    /* leave then scope */
    sema_if_stmt_leave_block(ctx);

    /* else? */
    if (consume_token_if(ctx, token_else) == NULL) {
        return sema_if_stmt(ctx, t, condition, then, NULL);
    }

    /* enter else scope */
    sema_if_stmt_enter_block(ctx);

    /* statement */
    else_ = parse_stmt(ctx);

    /* leave else scope */
    sema_if_stmt_leave_block(ctx);

    /* make node */
    return sema_if_stmt(ctx, t, condition, then, else_);
}

void parse_switch_stmt_case(ParserContext *ctx, ExprNode **case_value,
                            StmtNode **case_) {
    const Token *t;
    Vec *stmts;

    /* case */
    t = expect_token(ctx, token_case);

    /* expression */
    *case_value = parse_expr(ctx);

    /* : */
    expect_token(ctx, ':');

    /* statement* */
    stmts = vec_new();

    while (current_token(ctx)->kind != '}' &&
           current_token(ctx)->kind != token_case &&
           current_token(ctx)->kind != token_default) {
        vec_push(stmts, parse_stmt(ctx));
    }

    /* make node */
    *case_ = sema_switch_stmt_case(ctx, t, *case_value,
                                   (StmtNode **)stmts->data, stmts->size);
}

StmtNode *parse_switch_stmt_default(ParserContext *ctx) {
    const Token *t;
    Vec *stmts;

    /* default */
    t = expect_token(ctx, token_default);

    /* : */
    expect_token(ctx, ':');

    /* statement* */
    stmts = vec_new();

    while (current_token(ctx)->kind != '}' &&
           current_token(ctx)->kind != token_case &&
           current_token(ctx)->kind != token_default) {
        vec_push(stmts, parse_stmt(ctx));
    }

    /* make node */
    return sema_switch_stmt_default(ctx, t, (StmtNode **)stmts->data,
                                    stmts->size);
}

StmtNode *parse_switch_stmt(ParserContext *ctx) {
    const Token *t;
    ExprNode *condition;
    Vec *case_values;
    Vec *cases;
    StmtNode *default_;

    /* switch */
    t = expect_token(ctx, token_switch);

    /* expression */
    condition = parse_paren_expr(ctx);

    /* enter switch scope */
    sema_switch_stmt_enter(ctx);

    /* { */
    expect_token(ctx, '{');

    /* switch labels */
    case_values = vec_new();
    cases = vec_new();
    default_ = NULL;

    while (current_token(ctx)->kind != '}') {
        if (current_token(ctx)->kind == token_case) {
            ExprNode *case_value;
            StmtNode *case_;

            /* case label */
            parse_switch_stmt_case(ctx, &case_value, &case_);

            vec_push(case_values, case_value);
            vec_push(cases, case_);
        } else if (current_token(ctx)->kind == token_default) {
            /* default label */
            default_ = parse_switch_stmt_default(ctx);
            break;
        } else {
            break;
        }
    }

    /* } */
    expect_token(ctx, '}');

    /* leave switch scope and make node */
    return sema_switch_stmt_leave(
        ctx, t, condition, (ExprNode **)case_values->data,
        (StmtNode **)cases->data, cases->size, default_);
}

StmtNode *parse_while_stmt(ParserContext *ctx) {
    const Token *t;
    ExprNode *condition;
    StmtNode *body;

    /* while */
    t = expect_token(ctx, token_while);

    /* ( expression ) */
    condition = parse_paren_expr(ctx);

    /* enter body scope */
    sema_while_stmt_enter_body(ctx);

    /* statement */
    body = parse_stmt(ctx);

    /* leave body scope and make node */
    return sema_while_stmt_leave_body(ctx, t, condition, body);
}

StmtNode *parse_do_stmt(ParserContext *ctx) {
    const Token *t;
    StmtNode *body;
    ExprNode *condition;

    /* do */
    t = expect_token(ctx, token_do);

    /* enter body scope */
    sema_do_stmt_enter_body(ctx);

    /* statement */
    body = parse_stmt(ctx);

    /* leave body scope */
    sema_do_stmt_leave_body(ctx);

    /* while */
    expect_token(ctx, token_while);

    /* ( expression ) */
    condition = parse_paren_expr(ctx);

    /* ; */
    expect_token(ctx, ';');

    /* leave body scope and make node */
    return sema_do_stmt(ctx, t, body, condition);
}

StmtNode *parse_for_stmt(ParserContext *ctx) {
    const Token *t;
    ExprNode *initialization;
    ExprNode *condition;
    ExprNode *continuation;
    StmtNode *body;

    /* for */
    t = expect_token(ctx, token_for);

    /* ( */
    expect_token(ctx, '(');

    /* initialization expression */
    initialization = NULL;

    if (current_token(ctx)->kind != ';') {
        initialization = parse_expr(ctx);
    }

    /* ; */
    expect_token(ctx, ';');

    /* condition expression */
    condition = NULL;

    if (current_token(ctx)->kind != ';') {
        condition = parse_expr(ctx);
    }

    /* ; */
    expect_token(ctx, ';');

    /* continuation expression */
    continuation = NULL;

    if (current_token(ctx)->kind != ')') {
        continuation = parse_expr(ctx);
    }

    /* ) */
    expect_token(ctx, ')');

    /* enter body scope */
    sema_for_stmt_enter_body(ctx);

    /* statement */
    body = parse_stmt(ctx);

    /* leave body scope and make node */
    return sema_for_stmt_leave_body(ctx, t, initialization, condition,
                                    continuation, body);
}

StmtNode *parse_break_stmt(ParserContext *ctx) {
    const Token *t;

    /* break */
    t = expect_token(ctx, token_break);

    /* ; */
    expect_token(ctx, ';');

    return sema_break_stmt(ctx, t);
}

StmtNode *parse_continue_stmt(ParserContext *ctx) {
    const Token *t;

    /* continue */
    t = expect_token(ctx, token_continue);

    /* ; */
    expect_token(ctx, ';');

    return sema_continue_stmt(ctx, t);
}

StmtNode *parse_decl_stmt(ParserContext *ctx) {
    const Token *t;
    DeclNode *decl;

    /* declaration */
    decl = parse_decl(ctx);

    /* ; */
    t = expect_token(ctx, ';');

    /* make node */
    return sema_decl_stmt(ctx, decl, t);
}

StmtNode *parse_expr_stmt(ParserContext *ctx) {
    ExprNode *expr;
    const Token *t;

    /* expression */
    expr = parse_expr(ctx);

    /* ; */
    t = expect_token(ctx, ';');

    /* make node */
    return sema_expr_stmt(ctx, expr, t);
}

StmtNode *parse_stmt(ParserContext *ctx) {
    assert(ctx != NULL);

    switch (current_token(ctx)->kind) {
    case '{':
        return parse_compound_stmt(ctx);

    case token_return:
        return parse_return_stmt(ctx);

    case token_if:
        return parse_if_stmt(ctx);

    case token_switch:
        return parse_switch_stmt(ctx);

    case token_while:
        return parse_while_stmt(ctx);

    case token_do:
        return parse_do_stmt(ctx);

    case token_for:
        return parse_for_stmt(ctx);

    case token_break:
        return parse_break_stmt(ctx);

    case token_continue:
        return parse_continue_stmt(ctx);

    default:
        if (is_declaration_specifier_token(ctx, current_token(ctx))) {
            return parse_decl_stmt(ctx);
        }
        return parse_expr_stmt(ctx);
    }
}

void parse_direct_declarator(ParserContext *ctx, Type **type, const Token **t) {
    (void)type;

    switch (current_token(ctx)->kind) {
    case token_identifier:
        *t = expect_token(ctx, token_identifier);
        break;

    default:
        fprintf(stderr, "error at line %d: expected declarator, but got %s\n",
                current_token(ctx)->line, current_token(ctx)->text);
        exit(1);
    }
}

void parse_array_declarator(ParserContext *ctx, Type **type) {
    ExprNode *size;

    /* [ */
    expect_token(ctx, '[');

    /* expression */
    size = parse_expr(ctx);

    /* ] */
    expect_token(ctx, ']');

    /* postfix declarator */
    parse_declarator_postfix(ctx, type);

    /* make array type */
    *type = sema_array_declarator(ctx, *type, size);
}

void parse_declarator_postfix(ParserContext *ctx, Type **type) {
    assert(ctx != NULL);
    assert(type != NULL);
    assert(*type != NULL);

    switch (current_token(ctx)->kind) {
    case '[':
        parse_array_declarator(ctx, type);
        break;

    default:
        return;
    }
}

void parse_declarator_prefix(ParserContext *ctx, Type **type) {
    /* pointer types */
    /* TODO: const pointer */
    while (consume_token_if(ctx, '*') != NULL) {
        *type = pointer_type_new(*type);
    }
}

void parse_abstract_declarator(ParserContext *ctx, Type **type) {
    assert(ctx != NULL);
    assert(type != NULL);
    assert(*type != NULL);

    parse_declarator_prefix(ctx, type);
}

void parse_declarator(ParserContext *ctx, Type **type, const Token **t) {
    assert(ctx != NULL);
    assert(type != NULL);
    assert(*type != NULL);
    assert(t != NULL);

    parse_declarator_prefix(ctx, type);

    parse_direct_declarator(ctx, type, t);
    parse_declarator_postfix(ctx, type);
}

DeclNode *parse_typedef(ParserContext *ctx) {
    const Token *t;
    const Token *identifier;
    Type *type;

    /* typedef */
    t = expect_token(ctx, token_typedef);

    /* type */
    type = parse_type(ctx);

    /* declarator */
    parse_declarator(ctx, &type, &identifier);

    /* register type symbol and make node */
    return sema_typedef(ctx, t, type, identifier);
}

DeclNode *parse_var_decl(ParserContext *ctx) {
    Type *type;
    const Token *t;

    /* type */
    type = parse_type(ctx);

    /* ;? */
    if (current_token(ctx)->kind == ';') {
        return NULL;
    }

    /* declarator */
    parse_declarator(ctx, &type, &t);

    /* register symbol and make node */
    return sema_var_decl(ctx, type, t);
}

DeclNode *parse_decl(ParserContext *ctx) {
    assert(ctx != NULL);

    switch (current_token(ctx)->kind) {
    case token_typedef:
        return parse_typedef(ctx);

    default:
        return parse_var_decl(ctx);
    }
}

VariableNode *parse_param(ParserContext *ctx) {
    Type *type;
    const Token *t;

    assert(ctx != NULL);

    /* type */
    type = parse_type(ctx);

    /* declarator */
    parse_declarator(ctx, &type, &t);

    if (is_array_type(type)) {
        /* array type parameter is treated as a pointer */
        type = pointer_type_new(array_element_type(type));
    } else if (is_function_type(type)) {
        /* function type parameter is treated as a pointer */
        type = pointer_type_new(type);
    }

    /* register symbol and make node */
    return sema_param(ctx, type, t);
}

DeclNode *parse_top_level_typedef(ParserContext *ctx) {
    DeclNode *decl;

    /* typedef */
    decl = parse_typedef(ctx);

    /* ; */
    expect_token(ctx, ';');

    return decl;
}

DeclNode *parse_extern(ParserContext *ctx) {
    const Token *t;
    const Token *identifier;
    Type *type;

    /* extern */
    t = expect_token(ctx, token_extern);

    /* type */
    type = parse_type(ctx);

    /* declarator */
    parse_declarator(ctx, &type, &identifier);

    /* ; */
    expect_token(ctx, ';');

    /* make node and register symbol */
    return sema_extern(ctx, t, type, identifier);
}

DeclNode *parse_function(ParserContext *ctx) {
    const Token *t;
    Type *return_type;
    Vec *params;
    StmtNode *body;
    bool var_args;

    FunctionNode *p;

    /* type */
    return_type = parse_type(ctx);

    /* ;? */
    if (consume_token_if(ctx, ';') != NULL) {
        return NULL;
    }

    /* FIXME: pointer */
    parse_declarator_prefix(ctx, &return_type);

    if (current_token(ctx)->kind == token_identifier &&
        peek_token(ctx)->kind != '(') {
        /* declarator */
        parse_declarator(ctx, &return_type, &t);

        /* ; */
        expect_token(ctx, ';');

        /* variable declaration */
        return sema_var_decl(ctx, return_type, t);
    }

    /* identifier */
    t = expect_token(ctx, token_identifier);

    /* ( */
    expect_token(ctx, '(');

    /* enter parameter scope */
    sema_function_enter_params(ctx);

    /* parameters */
    params = vec_new();
    var_args = false;

    if (current_token(ctx)->kind == token_void &&
        peek_token(ctx)->kind == ')') {
        /* void */
        consume_token(ctx);
    } else {
        /* param {, param} */
        /* param */
        vec_push(params, parse_param(ctx));

        while (consume_token_if(ctx, ',') != NULL) {
            /* ...? */
            if (consume_token_if(ctx, token_var_args) != NULL) {
                var_args = true;
                break;
            }

            /* param */
            vec_push(params, parse_param(ctx));
        }
    }

    /* ) */
    expect_token(ctx, ')');

    /* leave parameter scope and make node */
    p = sema_function_leave_params(ctx, return_type, t,
                                   (VariableNode **)params->data, params->size,
                                   var_args);

    /* ;? */
    if (consume_token_if(ctx, ';') != NULL) {
        return (DeclNode *)p;
    }

    /* enter function body */
    sema_function_enter_body(ctx, p);

    /* body */
    body = parse_compound_stmt(ctx);

    /* leave function body */
    return (DeclNode *)sema_function_leave_body(ctx, p, body);
}

DeclNode *parse_top_level(ParserContext *ctx) {
    assert(ctx != NULL);

    switch (current_token(ctx)->kind) {
    case token_typedef:
        return parse_top_level_typedef(ctx);

    case token_extern:
        return parse_extern(ctx);

    default:
        return parse_function(ctx);
    }
}

TranslationUnitNode *parse(const char *filename, const char *src,
                           Vec *include_directories) {
    const Token **tokens;
    ParserContext *ctx;
    DeclNode *decl;
    Vec *decls;

    assert(filename != NULL);
    assert(src != NULL);
    assert(include_directories != NULL);

    /* get tokens */
    tokens =
        (const Token **)preprocess(filename, src, include_directories)->data;

    /* enter translation unit */
    ctx = sema_translation_unit_enter(tokens);

    /* top level declarations */
    decls = vec_new();

    while (current_token(ctx)->kind != '\0') {
        decl = parse_top_level(ctx);

        if (decl) {
            vec_push(decls, decl);
        }
    }

    /* make node */
    return sema_translation_unit_leave(ctx, filename, (DeclNode **)decls->data,
                                       decls->size);
}
