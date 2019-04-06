#include "nocc.h"

void test_parsing_integer(void) {
    const Token *tokens[] = {
        &(Token){token_number, "42", 1},
        &(Token){'\0', "", 1},
    };
    int index = 0;

    ExprNode *p = parse_expr(tokens, &index);
    IntegerNode *q = (IntegerNode *)p;

    assert(p->kind == node_integer);
    assert(p->line == 1);
    assert(q->value == 42);
}

void test_parsing_identifier(void) {
    const Token *tokens[] = {
        &(Token){token_identifier, "xyz", 1},
        &(Token){'\0', "", 1},
    };
    int index = 0;

    ExprNode *p = parse_expr(tokens, &index);
    IdentifierNode *q = (IdentifierNode *)p;

    assert(p->kind == node_identifier);
    assert(p->line == 1);
    assert(strcmp(q->identifier, "xyz") == 0);
}

void test_parsing_negative(void) {
    const Token *tokens[] = {
        &(Token){'-', "-", 1},
        &(Token){token_number, "10", 2},
        &(Token){'\0', "", 2},
    };
    int index = 0;

    ExprNode *p = parse_expr(tokens, &index);
    UnaryNode *q = (UnaryNode *)p;

    assert(p->kind == node_unary);
    assert(p->line == 1);
    assert(q->operator_ == '-');

    ExprNode *child = q->operand;
    IntegerNode *integer = (IntegerNode *)child;

    assert(child->kind == node_integer);
    assert(child->line == 2);
    assert(integer->value == 10);
}

void test_parsing_addition(void) {
    const Token *tokens[] = {
        &(Token){token_number, "6", 1},
        &(Token){'+', "+", 1},
        &(Token){token_number, "12", 2},
        &(Token){'\0', "", 2},
    };
    int index = 0;

    ExprNode *p = parse_expr(tokens, &index);
    BinaryNode *q = (BinaryNode *)p;

    assert(p->kind == node_binary);
    assert(p->line == 1);
    assert(q->operator_ == '+');

    IntegerNode *left = (IntegerNode *)q->left;
    IntegerNode *right = (IntegerNode *)q->right;

    assert(q->left->kind == node_integer);
    assert(q->left->line == 1);
    assert(left->value == 6);

    assert(q->right->kind == node_integer);
    assert(q->right->line == 2);
    assert(right->value == 12);
}

void test_parsing_multiplication(void) {
    const Token *tokens[] = {
        &(Token){token_number, "6", 1}, &(Token){'+', "+", 1},
        &(Token){token_number, "4", 1}, &(Token){'*', "*", 1},
        &(Token){token_number, "3", 1}, &(Token){'\0', "", 2},
    };
    int index = 0;

    ExprNode *p = parse_expr(tokens, &index);
    BinaryNode *q = (BinaryNode *)p;

    assert(p->kind == node_binary);
    assert(p->line == 1);
    assert(q->operator_ == '+');

    IntegerNode *left = (IntegerNode *)q->left;
    BinaryNode *right = (BinaryNode *)q->right;

    assert(q->left->kind == node_integer);
    assert(q->left->line == 1);
    assert(left->value == 6);

    assert(q->right->kind == node_binary);
    assert(q->right->line == 1);
    assert(right->operator_ == '*');
    assert(right->left->kind == node_integer);
    assert(right->right->kind == node_integer);
}

void test_parsing_expr_stmt(void) {
    const Token *tokens[] = {
        &(Token){token_number, "42", 1},
        &(Token){';', ";", 1},
        &(Token){'\0', "", 2},
    };
    int index = 0;

    StmtNode *p = parse_stmt(tokens, &index);
    ExprStmtNode *q = (ExprStmtNode *)p;

    assert(p->kind == node_expr);
    assert(p->line == 1);
    assert(q->expr->kind == node_integer);
}

void test_parsing_return_stmt(void) {
    const Token *tokens[] = {
        &(Token){token_return, "return", 1},
        &(Token){token_number, "42", 1},
        &(Token){';', ";", 1},
        &(Token){'\0', "", 2},
    };
    int index = 0;

    StmtNode *p = parse_stmt(tokens, &index);
    ReturnNode *q = (ReturnNode *)p;

    assert(p->kind == node_return);
    assert(p->line == 1);
    assert(q->return_value != NULL);
    assert(q->return_value->kind == node_integer);
}

void test_parsing_return_void_stmt(void) {
    const Token *tokens[] = {
        &(Token){token_return, "return", 1},
        &(Token){';', ";", 1},
        &(Token){'\0', "", 2},
    };
    int index = 0;

    StmtNode *p = parse_stmt(tokens, &index);
    ReturnNode *q = (ReturnNode *)p;

    assert(p->kind == node_return);
    assert(p->line == 1);
    assert(q->return_value == NULL);
}

void test_parsing_compound_stmt(void) {
    struct {
        int kind;
    } const suites[] = {
        {node_expr},
        {node_return},
    };
    const int len_suites = sizeof(suites) / sizeof(suites[0]);

    const Token *tokens[] = {
        &(Token){'{', "{", 1}, &(Token){token_number, "42", 1},
        &(Token){';', ";", 1}, &(Token){token_return, "return", 1},
        &(Token){';', ";", 1}, &(Token){'}', "}", 1},
        &(Token){'\0', "", 2},
    };
    int index = 0;

    StmtNode *p = parse_stmt(tokens, &index);
    CompoundNode *q = (CompoundNode *)p;

    assert(p->kind == node_compound);
    assert(p->line == 1);
    assert(q->stmts->size == len_suites);

    for (int i = 0; i < len_suites; i++) {
        StmtNode *s = q->stmts->data[i];

        if (s->kind != suites[i].kind) {
            fprintf(stderr, "%d: s->kind is expected %d, but got %d\n", i,
                    suites[i].kind, s->kind);
            exit(1);
        }
    }
}

void test_parser(void) {
    test_parsing_integer();
    test_parsing_identifier();
    test_parsing_negative();
    test_parsing_addition();
    test_parsing_multiplication();

    test_parsing_expr_stmt();
    test_parsing_return_stmt();
    test_parsing_return_void_stmt();
    test_parsing_compound_stmt();
}
