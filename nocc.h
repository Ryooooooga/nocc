#ifndef INCLUDE_nocc_h
#define INCLUDE_nocc_h

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

char *str_dup(const char *s);
char *str_dup_n(const char *s, int length);

struct Vec {
    int capacity;
    int size;
    void **data;
};

typedef struct Vec Vec;

Vec *vec_new(void);
void vec_reserve(Vec *v, int capacity);
void vec_resize(Vec *v, int size);
void *vec_back(Vec *v);
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

enum {
    token_number = 256,
    token_string,
    token_identifier,
    token_if,
    token_else,
    token_while,
    token_do,
    token_for,
    token_return,
    token_break,
    token_continue,
    token_void,
    token_char,
    token_int,
    token_const,
    token_struct,
    token_typedef,
    token_lesser_equal,
    token_greater_equal,
    token_equal,
    token_not_equal,
};

struct Token {
    int kind;
    char *text;
    int line;
    char *string;
    int len_string;
};

typedef struct Token Token;

Vec *lex(const char *src);

enum {
    type_void,
    type_int8,
    type_int32,
    type_pointer,
    type_array,
    type_function,
    type_struct,
};

typedef struct Type Type;
typedef struct PointerType PointerType;
typedef struct ArrayType ArrayType;
typedef struct FunctionType FunctionType;
typedef struct StructType StructType;

struct Type {
    int kind;
};

struct PointerType {
    int kind;
    Type *element_type;
};

struct ArrayType {
    int kind;
    Type *element_type;
    int length;
};

struct FunctionType {
    int kind;
    Type *return_type;
    Type **param_types;
    int num_params;
    bool var_args;
};

struct StructType {
    int kind;
    int line;
    char *identifier;
    struct MemberNode **members;
    int num_members;
    bool is_incomplete;
    LLVMTypeRef generated_type;
};

Type *type_get_void(void);
Type *type_get_int8(void);
Type *type_get_int32(void);
Type *pointer_type_new(Type *element_type);
Type *array_type_new(Type *element_type, int length);
Type *function_type_new(Type *return_type, Type **param_types, int num_params,
                        bool var_args);

bool type_equals(Type *a, Type *b);
bool is_void_type(Type *t);
bool is_int8_type(Type *t);
bool is_int32_type(Type *t);
bool is_pointer_type(Type *t);
bool is_array_type(Type *t);
bool is_function_type(Type *t);
bool is_struct_type(Type *t);
bool is_incomplete_type(Type *t);
bool is_void_pointer_type(Type *t);
bool is_function_pointer_type(Type *t);
bool is_incomplete_pointer_type(Type *t);

Type *pointer_element_type(Type *t);
Type *array_element_type(Type *t);
int array_type_count_elements(Type *t);
Type *function_return_type(Type *t);
int function_count_param_types(Type *t);
Type *function_param_type(Type *t, int index);
int struct_type_count_members(Type *t);
struct MemberNode *struct_type_member(Type *t, int index);
struct MemberNode *struct_type_find_member(Type *t, const char *member_name,
                                           int *index);

enum {
    node_integer,
    node_string,
    node_identifier,
    node_call,
    node_unary,
    node_cast,
    node_binary,
    node_dot,

    node_compound,
    node_return,
    node_if,
    node_while,
    node_do,
    node_for,
    node_break,
    node_continue,
    node_decl,
    node_expr,

    node_typedef,
    node_member,
    node_variable,
    node_param,
    node_function,
};

typedef struct ExprNode ExprNode;
typedef struct IntegerNode IntegerNode;
typedef struct StringNode StringNode;
typedef struct IdentifierNode IdentifierNode;
typedef struct CallNode CallNode;
typedef struct UnaryNode UnaryNode;
typedef struct CastNode CastNode;
typedef struct BinaryNode BinaryNode;
typedef struct DotNode DotNode;

typedef struct StmtNode StmtNode;
typedef struct CompoundNode CompoundNode;
typedef struct ReturnNode ReturnNode;
typedef struct IfNode IfNode;
typedef struct WhileNode WhileNode;
typedef struct DoNode DoNode;
typedef struct ForNode ForNode;
typedef struct BreakNode BreakNode;
typedef struct ContinueNode ContinueNode;
typedef struct DeclStmtNode DeclStmtNode;
typedef struct ExprStmtNode ExprStmtNode;

typedef struct DeclNode DeclNode;
typedef struct TypedefNode TypedefNode;
typedef struct MemberNode MemberNode;
typedef struct VariableNode VariableNode;
typedef struct ParamNode ParamNode;
typedef struct FunctionNode FunctionNode;

typedef struct TranslationUnitNode TranslationUnitNode;

struct ExprNode {
    int kind;
    int line;
    Type *type;
    bool is_lvalue;
};

struct IntegerNode {
    int kind;
    int line;
    Type *type;
    bool is_lvalue;
    int value;
};

struct StringNode {
    int kind;
    int line;
    Type *type;
    bool is_lvalue;
    char *string;
    int len_string;
};

struct IdentifierNode {
    int kind;
    int line;
    Type *type;
    bool is_lvalue;
    char *identifier;
    DeclNode *declaration;
};

struct CallNode {
    int kind;
    int line;
    Type *type;
    bool is_lvalue;
    ExprNode *callee;
    ExprNode **args;
    int num_args;
};

struct UnaryNode {
    int kind;
    int line;
    Type *type;
    bool is_lvalue;
    int operator_;
    ExprNode *operand;
};

struct CastNode {
    int kind;
    int line;
    Type *type;
    bool is_lvalue;
    ExprNode *operand;
};

struct BinaryNode {
    int kind;
    int line;
    Type *type;
    bool is_lvalue;
    int operator_;
    ExprNode *left;
    ExprNode *right;
};

struct DotNode {
    int kind;
    int line;
    Type *type;
    bool is_lvalue;
    ExprNode *parent;
    char *identifier;
    int index;
};

struct StmtNode {
    int kind;
    int line;
};

struct CompoundNode {
    int kind;
    int line;
    StmtNode **stmts;
    int num_stmts;
};

struct ReturnNode {
    int kind;
    int line;
    ExprNode *return_value;
};

struct IfNode {
    int kind;
    int line;
    ExprNode *condition;
    StmtNode *then;
    StmtNode *else_;
};

struct WhileNode {
    int kind;
    int line;
    ExprNode *condition;
    StmtNode *body;
};

struct DoNode {
    int kind;
    int line;
    StmtNode *body;
    ExprNode *condition;
};

struct ForNode {
    int kind;
    int line;
    ExprNode *initialization;
    ExprNode *condition;
    ExprNode *continuation;
    StmtNode *body;
};

struct BreakNode {
    int kind;
    int line;
};

struct ContinueNode {
    int kind;
    int line;
};

struct DeclStmtNode {
    int kind;
    int line;
    DeclNode *decl;
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
    LLVMValueRef generated_location;
};

struct TypedefNode {
    int kind;
    int line;
    char *identifier;
    Type *type;
    LLVMValueRef generated_location; /* unused */
};

struct MemberNode {
    int kind;
    int line;
    char *identifier;
    Type *type;
    LLVMValueRef generated_location; /* unused */
};

struct VariableNode {
    int kind;
    int line;
    char *identifier;
    Type *type;
    LLVMValueRef generated_location;
};

struct ParamNode {
    int kind;
    int line;
    char *identifier;
    Type *type;
    LLVMValueRef generated_location;
};

struct FunctionNode {
    int kind;
    int line;
    char *identifier;
    Type *type;
    LLVMValueRef generated_location;
    ParamNode **params;
    int num_params;
    bool var_args;
    StmtNode *body;
    DeclNode **locals;
    int num_locals;
};

struct TranslationUnitNode {
    char *filename;
    DeclNode **decls;
    int num_decls;
};

struct ScopeStack {
    Vec *scopes;
};

typedef struct ScopeStack ScopeStack;

ScopeStack *scope_stack_new(void);
int scope_stack_depth(ScopeStack *s);
void scope_stack_push(ScopeStack *s);
void scope_stack_pop(ScopeStack *s);
void *scope_stack_find(ScopeStack *s, const char *name, bool recursive);
void scope_stack_register(ScopeStack *s, const char *name, void *value);

struct ParserContext {
    ScopeStack *env;
    ScopeStack *struct_env;
    FunctionNode *current_function;
    Vec *locals;
    Vec *flow_state;
    const Token **tokens;
    int index;
};

typedef struct ParserContext ParserContext;

Type *parse_type(ParserContext *ctx);
ExprNode *parse_unary_expr(ParserContext *ctx);
ExprNode *parse_assign_expr(ParserContext *ctx);
ExprNode *parse_expr(ParserContext *ctx);
StmtNode *parse_stmt(ParserContext *ctx);
DeclNode *parse_decl(ParserContext *ctx);
ParamNode *parse_param(ParserContext *ctx);
DeclNode *parse_top_level(ParserContext *ctx);
TranslationUnitNode *parse(const char *filename, const char *src);

Type *sema_identifier_type(ParserContext *ctx, const Token *t);
MemberNode *sema_struct_member(ParserContext *ctx, Type *type, const Token *t);
Type *sema_struct_type_without_body(ParserContext *ctx, const Token *t,
                                    const Token *identifier);
StructType *sema_struct_type_enter(ParserContext *ctx, const Token *t,
                                   const Token *identifier);
Type *sema_struct_type_leave(ParserContext *ctx, StructType *type,
                             MemberNode **members, int num_members);

ExprNode *sema_paren_expr(ParserContext *ctx, const Token *open, ExprNode *expr,
                          const Token *close);
ExprNode *sema_integer_expr(ParserContext *ctx, const Token *t, int value);
ExprNode *sema_string_expr(ParserContext *ctx, const Token *t,
                           const char *string, int length);
ExprNode *sema_identifier_expr(ParserContext *ctx, const Token *t);
ExprNode *sema_call_expr(ParserContext *ctx, ExprNode *callee,
                         const Token *open, ExprNode **args, int num_args,
                         const Token *close);
ExprNode *sema_dot_expr(ParserContext *ctx, ExprNode *parent, const Token *t,
                        const Token *identifier);
ExprNode *sema_unary_expr(ParserContext *ctx, const Token *t,
                          ExprNode *operand);
ExprNode *sema_cast_expr(ParserContext *ctx, const Token *open, Type *type,
                         const Token *close, ExprNode *operand);
ExprNode *sema_binary_expr(ParserContext *ctx, ExprNode *left, const Token *t,
                           ExprNode *right);

void sema_compound_stmt_enter(ParserContext *ctx);
StmtNode *sema_compound_stmt_leave(ParserContext *ctx, const Token *open,
                                   StmtNode **stmts, int num_stmts,
                                   const Token *close);
StmtNode *sema_return_stmt(ParserContext *ctx, const Token *t,
                           ExprNode *return_value, const Token *semi);
void sema_if_stmt_enter_block(ParserContext *ctx);
void sema_if_stmt_leave_block(ParserContext *ctx);
StmtNode *sema_if_stmt(ParserContext *ctx, const Token *t, ExprNode *condition,
                       StmtNode *then, StmtNode *else_);
void sema_while_stmt_enter_body(ParserContext *ctx);
StmtNode *sema_while_stmt_leave_body(ParserContext *ctx, const Token *t,
                                     ExprNode *condition, StmtNode *body);
void sema_do_stmt_enter_body(ParserContext *ctx);
void sema_do_stmt_leave_body(ParserContext *ctx);
StmtNode *sema_do_stmt(ParserContext *ctx, const Token *t, StmtNode *body,
                       ExprNode *condition);
void sema_for_stmt_enter_body(ParserContext *ctx);
StmtNode *sema_for_stmt_leave_body(ParserContext *ctx, const Token *t,
                                   ExprNode *initialization,
                                   ExprNode *condition, ExprNode *continuation,
                                   StmtNode *body);
StmtNode *sema_break_stmt(ParserContext *ctx, const Token *t);
StmtNode *sema_continue_stmt(ParserContext *ctx, const Token *t);
StmtNode *sema_decl_stmt(ParserContext *ctx, DeclNode *decl, const Token *t);
StmtNode *sema_expr_stmt(ParserContext *ctx, ExprNode *expr, const Token *t);

DeclNode *sema_typedef(ParserContext *ctx, const Token *t, Type *type,
                       const Token *identifier);
DeclNode *sema_var_decl(ParserContext *ctx, Type *type, const Token *t);
ParamNode *sema_param(ParserContext *ctx, Type *type, const Token *t);

void sema_function_enter_params(ParserContext *ctx);
FunctionNode *sema_function_leave_params(ParserContext *ctx, Type *return_type,
                                         const Token *t, ParamNode **params,
                                         int num_params, bool var_args);
void sema_function_enter_body(ParserContext *ctx, FunctionNode *p);
FunctionNode *sema_function_leave_body(ParserContext *ctx, FunctionNode *p,
                                       StmtNode *body);

ParserContext *sema_translation_unit_enter(const char *src);
TranslationUnitNode *sema_translation_unit_leave(ParserContext *ctx,
                                                 const char *filename,
                                                 DeclNode **decls,
                                                 int num_decls);

struct GeneratorContext {
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    Vec *break_targets;
    Vec *continue_targets;
};

typedef struct GeneratorContext GeneratorContext;

LLVMTypeRef generate_type(GeneratorContext *ctx, Type *p);
LLVMValueRef generate_expr(GeneratorContext *ctx, ExprNode *p);
LLVMValueRef generate_expr_addr(GeneratorContext *ctx, ExprNode *p);
bool generate_stmt(GeneratorContext *ctx, StmtNode *p);

LLVMValueRef generate_function(GeneratorContext *ctx, FunctionNode *p);
void generate_decl(GeneratorContext *ctx, DeclNode *p);
LLVMModuleRef generate(TranslationUnitNode *p);

#endif
