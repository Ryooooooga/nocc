#ifndef INCLUDE_nocc_h
#define INCLUDE_nocc_h

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

enum { token_number = 256, token_identifier, token_if };

struct Vec {
    int capacity;
    int size;
    void **data;
};

typedef struct Vec Vec;

Vec *vec_new(void);
void vec_reserve(Vec *v, int capacity);
void vec_resize(Vec *v, int size);
void vec_push(Vec *v, void *value);
void *vec_pop(Vec *v);

struct Token {
    int kind;
    char *text;
    int line;
};

typedef struct Token Token;

Vec *lex(const char *src);

enum { node_integer, node_identifier, node_unary, node_binary };

typedef struct Node Node;
typedef struct IntegerNode IntegerNode;
typedef struct IdentifierNode IdentifierNode;
typedef struct UnaryNode UnaryNode;
typedef struct BinaryNode BinaryNode;

struct Node {
    int kind;
    int line;
};

struct IntegerNode {
    int kind;
    int line;
    int value;
};

struct IdentifierNode {
    int kind;
    int line;
    char *identifier;
};

struct UnaryNode {
    int kind;
    int line;
    int operator_;
    Node *operand;
};

struct BinaryNode {
    int kind;
    int line;
    int operator_;
    Node *left;
    Node *right;
};

Node *parse_expr(const Token **toks, int *n);

struct GeneratorContext {
    LLVMBuilderRef builder;
};

typedef struct GeneratorContext GeneratorContext;

LLVMValueRef generate_expr(GeneratorContext *ctx, Node *p);

#endif
