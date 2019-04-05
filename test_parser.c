#include "nocc.h"

void test_parsing_integer(void) {
    const Token *tokens[] = {
        &(Token){token_number, "42", 1},
        &(Token){'\0', "", 1},
    };
    int index = 0;

    Node *p = parse_expr(tokens, &index);
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

    Node *p = parse_expr(tokens, &index);
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

    Node *p = parse_expr(tokens, &index);
    UnaryNode *q = (UnaryNode *)p;

    assert(p->kind == node_unary);
    assert(p->line == 1);
    assert(q->operator_ == '-');

    Node *child = q->operand;
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

    Node *p = parse_expr(tokens, &index);
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

    Node *p = parse_expr(tokens, &index);
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

void test_parser(void) {
    test_parsing_integer();
    test_parsing_identifier();
    test_parsing_negative();
    test_parsing_addition();
    test_parsing_multiplication();
}
