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

enum {
    token_number = 256,
    token_identifier,
    token_if,
    token_return,
    token_void,
    token_int,
};

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

struct Map {
    Vec *keys;
    Vec *values;
};

typedef struct Map Map;

Map *map_new(void);
int map_size(Map *m);
bool map_contains(Map *m, const char *k);
void *map_get(Map *m, const char *k);
void map_add(Map *m, const char *k, void *v);

struct Token {
    int kind;
    char *text;
    int line;
};

typedef struct Token Token;

Vec *lex(const char *src);

enum {
    type_void,
    type_int32,
};

struct Type {
    int kind;
};

typedef struct Type Type;

Type *type_get_void(void);
Type *type_get_int32(void);

enum {
    node_integer,
    node_identifier,
    node_unary,
    node_binary,

    node_compound,
    node_return,
    node_expr,

    node_param,
    node_function,
};

typedef struct ExprNode ExprNode;
typedef struct IntegerNode IntegerNode;
typedef struct IdentifierNode IdentifierNode;
typedef struct UnaryNode UnaryNode;
typedef struct BinaryNode BinaryNode;

typedef struct StmtNode StmtNode;
typedef struct CompoundNode CompoundNode;
typedef struct ReturnNode ReturnNode;
typedef struct ExprStmtNode ExprStmtNode;

typedef struct DeclNode DeclNode;
typedef struct ParamNode ParamNode;
typedef struct FunctionNode FunctionNode;

typedef struct TranslationUnitNode TranslationUnitNode;

struct ExprNode {
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
    ExprNode *operand;
};

struct BinaryNode {
    int kind;
    int line;
    int operator_;
    ExprNode *left;
    ExprNode *right;
};

struct StmtNode {
    int kind;
    int line;
};

struct CompoundNode {
    int kind;
    int line;
    Vec *stmts;
};

struct ReturnNode {
    int kind;
    int line;
    ExprNode *return_value;
};

struct ExprStmtNode {
    int kind;
    int line;
    ExprNode *expr;
};

struct DeclNode {
    int kind;
    int line;
    char *identifier;
    Type *type;
};

struct ParamNode {
    int kind;
    int line;
    char *identifier;
    Type *type;
};

struct FunctionNode {
    int kind;
    int line;
    char *identifier;
    Type *return_type;
    Vec *params;
    bool var_args;
    StmtNode *body;
};

struct TranslationUnitNode {
    char *filename;
    Vec *decls;
};

Type *parse_type(const Token **toks, int *n);
ExprNode *parse_expr(const Token **toks, int *n);
StmtNode *parse_stmt(const Token **toks, int *n);
ParamNode *parse_param(const Token **toks, int *n);
DeclNode *parse_top_level(const Token **toks, int *n);
TranslationUnitNode *parse(const char *filename, const char *src);

struct GeneratorContext {
    LLVMModuleRef module;
    LLVMBuilderRef builder;
};

typedef struct GeneratorContext GeneratorContext;

LLVMTypeRef generate_type(GeneratorContext *ctx, Type *p);
LLVMValueRef generate_expr(GeneratorContext *ctx, ExprNode *p);
bool generate_stmt(GeneratorContext *ctx, StmtNode *p);

LLVMValueRef generate_function(GeneratorContext *ctx, FunctionNode *p);
void generate_decl(GeneratorContext *ctx, DeclNode *p);
LLVMModuleRef generate(TranslationUnitNode *p);

#endif
