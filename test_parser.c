#include "nocc.h"

Token *test_token_new(int kind, char *text, char *string) {
    Token *t;

    assert(text != NULL);

    t = malloc(sizeof(*t));
    t->kind = kind;
    t->text = text;
    t->filename = str_dup("test_lexer");
    t->line = 1;
    t->string = string;
    t->len_string = string == NULL ? 0 : strlen(string);

    return t;
}

void test_parsing_type_void(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens =
            (const Token *[]){
                test_token_new(token_void, "void", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    Type *p = parse_type(ctx);

    assert(is_void_type(p));
}

void test_parsing_type_int(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens =
            (const Token *[]){
                test_token_new(token_int, "int", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    Type *p = parse_type(ctx);

    assert(is_int32_type(p));
}

void test_parsing_integer(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens =
            (const Token *[]){
                test_token_new(token_number, "42", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    IntegerNode *p = (IntegerNode *)parse_expr(ctx);

    assert(p->kind == node_integer);
    assert(is_int32_type(p->type));
    assert(p->value == 42);
}

void test_parsing_identifier(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens =
            (const Token *[]){
                test_token_new(token_identifier, "xyz", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    ParamNode *decl = &(ParamNode){
        .kind = node_param,
        .line = 1,
        .identifier = "xyz",
        .type = type_get_int32(),
    };

    scope_stack_register(ctx->env, decl->identifier, decl);

    IdentifierNode *p = (IdentifierNode *)parse_expr(ctx);

    assert(p->kind == node_identifier);
    assert(is_int32_type(p->type));
    assert(strcmp(p->identifier, "xyz") == 0);
    assert(p->declaration == (DeclNode *)decl);
}

void test_parsing_call(void) {
    Vec *toks = preprocess("test_parsing_call", "f()", vec_new());

    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens = (const Token **)toks->data,
        .index = 0,
    };

    FunctionNode *decl = &(FunctionNode){
        .kind = node_function,
        .line = 1,
        .identifier = "f",
        .type = function_type_new(type_get_int32(), NULL, 0, false),
        .params = NULL,
        .num_params = 0,
        .var_args = false,
        .body = NULL,
    };

    scope_stack_register(ctx->env, decl->identifier, decl);

    CallNode *p = (CallNode *)parse_expr(ctx);
    CastNode *callee = (CastNode *)p->callee;

    assert(p->kind == node_call);
    assert(is_int32_type(p->type));
    assert(p->num_args == 0);

    assert(callee->kind == node_cast);
    assert(is_function_pointer_type(callee->type));

    assert(callee->operand->kind == node_identifier);
    assert(strcmp(((IdentifierNode *)callee->operand)->identifier, "f") == 0);
}

void test_parsing_call_arg(void) {
    Vec *toks = preprocess("test_parsing_call_arg", "f(42)", vec_new());

    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens = (const Token **)toks->data,
        .index = 0,
    };

    FunctionNode *decl = &(FunctionNode){
        .kind = node_function,
        .line = 1,
        .identifier = "f",
        .type = function_type_new(type_get_int32(),
                                  (Type *[]){
                                      type_get_int32(),
                                  },
                                  1, false),
        .params = NULL,
        .num_params = 0,
        .var_args = false,
        .body = NULL,
    };

    scope_stack_register(ctx->env, decl->identifier, decl);

    CallNode *p = (CallNode *)parse_expr(ctx);
    CastNode *callee = (CastNode *)p->callee;

    assert(p->kind == node_call);
    assert(is_int32_type(p->type));
    assert(p->num_args == 1);

    assert(callee->kind == node_cast);
    assert(is_function_pointer_type(callee->type));

    assert(callee->operand->kind == node_identifier);
    assert(strcmp(((IdentifierNode *)callee->operand)->identifier, "f") == 0);
}

void test_parsing_call_args(void) {
    Vec *toks = preprocess("test_parsing_call_args", "f(1, 2, 3)", vec_new());

    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens = (const Token **)toks->data,
        .index = 0,
    };

    FunctionNode *decl = &(FunctionNode){
        .kind = node_function,
        .line = 1,
        .identifier = "f",
        .type = function_type_new(type_get_int32(),
                                  (Type *[]){
                                      type_get_int32(),
                                      type_get_int32(),
                                      type_get_int32(),
                                  },
                                  3, false),
        .params = NULL,
        .num_params = 0,
        .var_args = false,
        .body = NULL,
    };

    scope_stack_register(ctx->env, decl->identifier, decl);

    CallNode *p = (CallNode *)parse_expr(ctx);
    CastNode *callee = (CastNode *)p->callee;

    assert(p->kind == node_call);
    assert(is_int32_type(p->type));
    assert(p->num_args == 3);

    assert(callee->kind == node_cast);
    assert(is_function_pointer_type(callee->type));

    assert(callee->operand->kind == node_identifier);
    assert(strcmp(((IdentifierNode *)callee->operand)->identifier, "f") == 0);
}

void test_parsing_negative(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens =
            (const Token *[]){
                test_token_new('-', "-", NULL),
                test_token_new(token_number, "10", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    UnaryNode *p = (UnaryNode *)parse_expr(ctx);

    assert(p->kind == node_unary);
    assert(is_int32_type(p->type));
    assert(p->operator_ == '-');

    IntegerNode *integer = (IntegerNode *)p->operand;

    assert(integer->kind == node_integer);
    assert(integer->value == 10);
}

void test_parsing_addition(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens =
            (const Token *[]){
                test_token_new(token_number, "6", NULL),
                test_token_new('+', "+", NULL),
                test_token_new(token_number, "12", NULL),
                test_token_new(token_number, "10", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    BinaryNode *p = (BinaryNode *)parse_expr(ctx);

    assert(p->kind == node_binary);
    assert(is_int32_type(p->type));
    assert(p->operator_ == '+');

    IntegerNode *left = (IntegerNode *)p->left;
    IntegerNode *right = (IntegerNode *)p->right;

    assert(left->kind == node_integer);
    assert(left->value == 6);

    assert(right->kind == node_integer);
    assert(right->value == 12);
}

void test_parsing_multiplication(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens =
            (const Token *[]){
                test_token_new(token_number, "6", NULL),
                test_token_new('+', "+", NULL),
                test_token_new(token_number, "4", NULL),
                test_token_new('*', "*", NULL),
                test_token_new(token_number, "3", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    BinaryNode *p = (BinaryNode *)parse_expr(ctx);

    assert(p->kind == node_binary);
    assert(is_int32_type(p->type));
    assert(p->operator_ == '+');

    IntegerNode *left = (IntegerNode *)p->left;
    BinaryNode *right = (BinaryNode *)p->right;

    assert(left->kind == node_integer);
    assert(left->value == 6);

    assert(right->kind == node_binary);
    assert(right->operator_ == '*');
    assert(right->left->kind == node_integer);
    assert(right->right->kind == node_integer);
}

void test_parsing_paren(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens =
            (const Token *[]){
                test_token_new('(', "(", NULL),
                test_token_new(token_number, "6", NULL),
                test_token_new('+', "+", NULL),
                test_token_new(token_number, "4", NULL),
                test_token_new(')', ")", NULL),
                test_token_new('*', "*", NULL),
                test_token_new(token_number, "3", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    BinaryNode *p = (BinaryNode *)parse_expr(ctx);

    assert(p->kind == node_binary);
    assert(is_int32_type(p->type));
    assert(p->operator_ == '*');

    BinaryNode *left = (BinaryNode *)p->left;
    IntegerNode *right = (IntegerNode *)p->right;

    assert(left->kind == node_binary);
    assert(left->operator_ == '+');
    assert(left->left->kind == node_integer);
    assert(left->right->kind == node_integer);

    assert(right->kind == node_integer);
    assert(right->value == 3);
}

void test_parsing_expr_stmt(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens =
            (const Token *[]){
                test_token_new(token_number, "42", NULL),
                test_token_new(';', ";", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    ExprStmtNode *p = (ExprStmtNode *)parse_stmt(ctx);

    assert(p->kind == node_expr);
    assert(p->line == 1);
    assert(p->expr->kind == node_integer);
}

void test_parsing_return_stmt(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .current_function =
            &(FunctionNode){
                .kind = node_function,
                .line = 1,
                .identifier = "f",
                .type = function_type_new(type_get_int32(), NULL, 0, false),
                .params = NULL,
                .num_params = 0,
                .var_args = false,
            },
        .tokens =
            (const Token *[]){
                test_token_new(token_return, "return", NULL),
                test_token_new(token_number, "42", NULL),
                test_token_new(';', ";", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    ReturnNode *p = (ReturnNode *)parse_stmt(ctx);

    assert(p->kind == node_return);
    assert(p->return_value != NULL);
    assert(p->return_value->kind == node_integer);
}

void test_parsing_return_void_stmt(void) {
    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .current_function =
            &(FunctionNode){
                .kind = node_function,
                .line = 1,
                .identifier = "f",
                .type = function_type_new(type_get_void(), NULL, 0, false),
                .params = NULL,
                .num_params = 0,
                .var_args = false,
            },
        .tokens =
            (const Token *[]){
                test_token_new(token_return, "return", NULL),
                test_token_new(';', ";", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    ReturnNode *p = (ReturnNode *)parse_stmt(ctx);

    assert(p->kind == node_return);
    assert(p->line == 1);
    assert(p->return_value == NULL);
}

void test_parsing_compound_stmt(void) {
    struct {
        int kind;
    } const suites[] = {
        {node_expr},
        {node_return},
    };
    const int len_suites = sizeof(suites) / sizeof(suites[0]);

    ParserContext *ctx = &(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .current_function =
            &(FunctionNode){
                .kind = node_function,
                .line = 1,
                .identifier = "f",
                .type = function_type_new(type_get_void(), NULL, 0, false),
                .params = NULL,
                .num_params = 0,
                .var_args = false,
            },
        .tokens =
            (const Token *[]){
                test_token_new('{', "{", NULL),
                test_token_new(token_number, "42", NULL),
                test_token_new(';', ";", NULL),
                test_token_new(token_return, "return", NULL),
                test_token_new(';', ";", NULL),
                test_token_new('}', "}", NULL),
                test_token_new('\0', "", NULL),
            },
        .index = 0,
    };

    CompoundNode *p = (CompoundNode *)parse_stmt(ctx);

    assert(p->kind == node_compound);
    assert(p->num_stmts == len_suites);

    for (int i = 0; i < len_suites; i++) {
        if (p->stmts[i]->kind != suites[i].kind) {
            fprintf(stderr,
                    "%d: p->stmts[i]->kind is expected %d, but got %d\n", i,
                    suites[i].kind, p->stmts[i]->kind);
            exit(1);
        }
    }
}

void test_parsing_if_stmt(void) {
    Vec *toks = preprocess("test_parsing_if_stmt", "if (42) {}", vec_new());

    IfNode *p = (IfNode *)parse_stmt(&(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens = (const Token **)toks->data,
        .index = 0,
    });

    assert(p->kind == node_if);
    assert(p->line == 1);
    assert(p->condition->kind == node_integer);
    assert(p->then->kind == node_compound);
    assert(p->else_ == NULL);
}

void test_parsing_if_else_stmt(void) {
    Vec *toks = preprocess("test_parsing_if_else_stmt", "if (42) {} else 42;",
                           vec_new());

    IfNode *p = (IfNode *)parse_stmt(&(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens = (const Token **)toks->data,
        .index = 0,
    });

    assert(p->kind == node_if);
    assert(p->line == 1);
    assert(p->condition->kind == node_integer);
    assert(p->then->kind == node_compound);
    assert(p->else_ != NULL);
    assert(p->else_->kind == node_expr);
}

void test_parsing_parameter(void) {
    Vec *toks = preprocess("test_parsing_parameter", "int a", vec_new());

    ParamNode *p = parse_param(&(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens = (const Token **)toks->data,
        .index = 0,
    });

    assert(p->kind == node_param);
    assert(p->line == 1);
    assert(strcmp(p->identifier, "a") == 0);
    assert(is_int32_type(p->type));
}

void test_parsing_function(void) {
    Vec *toks = preprocess("test_parsing_function",
                           "int main(void) { return 42; }", vec_new());

    DeclNode *p = parse_top_level(&(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens = (const Token **)toks->data,
        .index = 0,
    });
    FunctionNode *q = (FunctionNode *)p;
    FunctionType *t = (FunctionType *)q->type;

    assert(p->kind == node_function);
    assert(p->line == 1);
    assert(strcmp(p->identifier, "main") == 0);
    assert(q->num_params == 0);
    assert(q->var_args == false);
    assert(q->body != NULL);
    assert(q->body->kind == node_compound);

    assert(t->kind == type_function);
    assert(is_int32_type(t->return_type));
    assert(t->num_params == 0);
}

void test_parsing_function_prototype(void) {
    Vec *toks = preprocess("test_parsing_function_prototype", "int main(void);",
                           vec_new());

    DeclNode *p = parse_top_level(&(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens = (const Token **)toks->data,
        .index = 0,
    });
    FunctionNode *q = (FunctionNode *)p;
    FunctionType *t = (FunctionType *)q->type;

    assert(p->kind == node_function);
    assert(p->line == 1);
    assert(strcmp(p->identifier, "main") == 0);
    assert(q->num_params == 0);
    assert(q->var_args == false);
    assert(q->body == NULL);

    assert(t->kind == type_function);
    assert(is_int32_type(t->return_type));
    assert(t->num_params == 0);
}

void test_parsing_function_param(void) {
    Vec *toks = preprocess("test_parsing_function_param", "int main(int a);",
                           vec_new());

    DeclNode *p = parse_top_level(&(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens = (const Token **)toks->data,
        .index = 0,
    });
    FunctionNode *q = (FunctionNode *)p;
    FunctionType *t = (FunctionType *)q->type;

    assert(p->kind == node_function);
    assert(p->line == 1);
    assert(strcmp(p->identifier, "main") == 0);
    assert(q->num_params == 1);
    assert(q->var_args == false);
    assert(q->body == NULL);

    assert(t->kind == type_function);
    assert(is_int32_type(t->return_type));
    assert(t->num_params == 1);
    assert(is_int32_type(t->param_types[0]));

    ParamNode *a = q->params[0];

    assert(a->kind == node_param);
    assert(a->line == 1);
    assert(strcmp(a->identifier, "a") == 0);
    assert(is_int32_type(a->type));
}

void test_parsing_function_params(void) {
    Vec *toks = preprocess("test_parsing_function_params",
                           "int main(int a, int b);", vec_new());

    DeclNode *p = parse_top_level(&(ParserContext){
        .env = scope_stack_new(),
        .struct_env = scope_stack_new(),
        .tokens = (const Token **)toks->data,
        .index = 0,
    });
    FunctionNode *q = (FunctionNode *)p;
    FunctionType *t = (FunctionType *)q->type;

    assert(p->kind == node_function);
    assert(p->line == 1);
    assert(strcmp(p->identifier, "main") == 0);
    assert(q->num_params == 2);
    assert(q->var_args == false);
    assert(q->body == NULL);

    assert(t->kind == type_function);
    assert(is_int32_type(t->return_type));
    assert(t->num_params == 2);
    assert(is_int32_type(t->param_types[0]));
    assert(is_int32_type(t->param_types[1]));

    ParamNode *a = q->params[0];
    ParamNode *b = q->params[1];

    assert(a->kind == node_param);
    assert(a->line == 1);
    assert(strcmp(a->identifier, "a") == 0);
    assert(is_int32_type(a->type));

    assert(b->kind == node_param);
    assert(b->line == 1);
    assert(strcmp(b->identifier, "b") == 0);
    assert(is_int32_type(b->type));
}

void test_parsing_translation_unit(void) {
    TranslationUnitNode *p = parse("test_parsing_translation_unit",
                                   "int main(void) {return 42;}", vec_new());

    assert(strcmp(p->filename, "test_parsing_translation_unit") == 0);
    assert(p->num_decls == 1);

    FunctionNode *f = (FunctionNode *)p->decls[0];
    FunctionType *t = (FunctionType *)f->type;

    assert(f->kind == node_function);
    assert(f->line == 1);
    assert(strcmp(f->identifier, "main") == 0);
    assert(f->num_params == 0);
    assert(f->var_args == false);
    assert(f->body != NULL);
    assert(f->body->kind == node_compound);

    assert(t->kind == type_function);
    assert(is_int32_type(t->return_type));
    assert(t->num_params == 0);
}

void test_parser(void) {
    test_parsing_type_void();
    test_parsing_type_int();

    test_parsing_integer();
    test_parsing_identifier();
    test_parsing_call();
    test_parsing_call_arg();
    test_parsing_call_args();
    test_parsing_negative();
    test_parsing_addition();
    test_parsing_multiplication();
    test_parsing_paren();

    test_parsing_expr_stmt();
    test_parsing_return_stmt();
    test_parsing_return_void_stmt();
    test_parsing_compound_stmt();
    test_parsing_if_stmt();
    test_parsing_if_else_stmt();

    test_parsing_parameter();
    test_parsing_function();
    test_parsing_function_prototype();
    test_parsing_function_param();
    test_parsing_function_params();

    test_parsing_translation_unit();
}
