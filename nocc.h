#ifndef INCLUDE_nocc_h
#define INCLUDE_nocc_h

#include "llvm.h"
#include "std.h"

#include "file.h"
#include "map.h"
#include "path.h"
#include "util.h"
#include "vec.h"

#define token_number 256
#define token_character 257
#define token_string 258
#define token_identifier 259
#define token_if 260
#define token_else 261
#define token_switch 262
#define token_case 263
#define token_default 264
#define token_while 265
#define token_do 266
#define token_for 267
#define token_return 268
#define token_break 269
#define token_continue 270
#define token_void 271
#define token_char 272
#define token_int 273
#define token_long 274
#define token_unsigned 275
#define token_const 276
#define token_struct 277
#define token_typedef 278
#define token_extern 279
#define token_sizeof 280
#define token_lesser_equal 281
#define token_greater_equal 282
#define token_equal 283
#define token_not_equal 284
#define token_increment 285
#define token_decrement 286
#define token_and 287
#define token_or 288
#define token_arrow 289
#define token_var_args 290

typedef struct Token {
    int kind;
    char *text;
    const char *filename;
    int line;
    char *string;
    int len_string;
} Token;

Vec *lex(const char *filename, const char *src);
Vec *preprocess(const char *filename, const char *src,
                Vec *include_directories);

#define type_void 0
#define type_int8 1
#define type_int32 2
#define type_pointer 3
#define type_array 4
#define type_function 5
#define type_struct 6

typedef struct Type {
    int kind;
} Type;

typedef struct PointerType {
    int kind;
    Type *element_type;
} PointerType;

typedef struct ArrayType {
    int kind;
    Type *element_type;
    int length;
} ArrayType;

typedef struct FunctionType {
    int kind;
    Type *return_type;
    Type **param_types;
    int num_params;
    bool var_args;
} FunctionType;

typedef struct StructType {
    int kind;
    struct TypeSymbol *symbol;
    struct MemberNode **members;
    int num_members;
    bool is_incomplete;
    LLVMTypeRef generated_type;
} StructType;

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
bool is_integer_type(Type *t);
bool is_scalar_type(Type *t);

Type *pointer_element_type(Type *t);
Type *array_element_type(Type *t);
int array_type_count_elements(Type *t);
Type *function_return_type(Type *t);
int function_count_param_types(Type *t);
Type *function_param_type(Type *t, int index);
bool function_type_is_var_args(Type *t);
int struct_type_count_members(Type *t);
struct MemberNode *struct_type_member(Type *t, int index);
struct MemberNode *struct_type_find_member(Type *t, const char *member_name,
                                           int *index);

#define symbol_variable 0
#define symbol_type 1

typedef struct Symbol {
    int kind;
    const char *filename;
    int line;
    const char *identifier;
    Type *type;
} Symbol;

typedef struct VariableSymbol {
    int kind;
    const char *filename;
    int line;
    const char *identifier;
    Type *type;
    LLVMValueRef generated_location;
} VariableSymbol;

typedef struct TypeSymbol {
    int kind;
    const char *filename;
    int line;
    const char *identifier;
    Type *type;
} TypeSymbol;

VariableSymbol *variable_symbol_new(const char *filename, int line,
                                    const char *identifier, Type *type);
TypeSymbol *type_symbol_new(const char *filename, int line,
                            const char *identifier, Type *type);

#define node_integer 0
#define node_string 1
#define node_identifier 2
#define node_postfix 3
#define node_call 4
#define node_unary 5
#define node_sizeof 6
#define node_cast 7
#define node_binary 8
#define node_dot 9

#define node_compound 10
#define node_return 11
#define node_if 12
#define node_switch 13
#define node_while 14
#define node_do 15
#define node_for 16
#define node_break 17
#define node_continue 18
#define node_decl 19
#define node_expr 20

#define node_typedef 21
#define node_extern 22
#define node_member 23
#define node_variable 24
#define node_function 26

typedef struct ExprNode ExprNode;
typedef struct IntegerNode IntegerNode;
typedef struct StringNode StringNode;
typedef struct IdentifierNode IdentifierNode;
typedef struct PostfixNode PostfixNode;
typedef struct CallNode CallNode;
typedef struct UnaryNode UnaryNode;
typedef struct SizeofNode SizeofNode;
typedef struct CastNode CastNode;
typedef struct BinaryNode BinaryNode;
typedef struct DotNode DotNode;

typedef struct StmtNode StmtNode;
typedef struct CompoundNode CompoundNode;
typedef struct ReturnNode ReturnNode;
typedef struct IfNode IfNode;
typedef struct SwitchNode SwitchNode;
typedef struct WhileNode WhileNode;
typedef struct DoNode DoNode;
typedef struct ForNode ForNode;
typedef struct BreakNode BreakNode;
typedef struct ContinueNode ContinueNode;
typedef struct DeclStmtNode DeclStmtNode;
typedef struct ExprStmtNode ExprStmtNode;

typedef struct DeclNode DeclNode;
typedef struct TypedefNode TypedefNode;
typedef struct ExternNode ExternNode;
typedef struct MemberNode MemberNode;
typedef struct VariableNode VariableNode;
typedef struct FunctionNode FunctionNode;

typedef struct TranslationUnitNode TranslationUnitNode;

struct ExprNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
};

struct IntegerNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
    int value;
};

struct StringNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
    char *string;
    int len_string;
};

struct IdentifierNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
    VariableSymbol *symbol;
};

struct PostfixNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
    ExprNode *operand;
    int operator_;
};

struct CallNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
    ExprNode *callee;
    ExprNode **args;
    int num_args;
};

struct UnaryNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
    int operator_;
    ExprNode *operand;
};

struct SizeofNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
    Type *operand;
};

struct CastNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
    ExprNode *operand;
};

struct BinaryNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
    int operator_;
    ExprNode *left;
    ExprNode *right;
};

struct DotNode {
    int kind;
    char *filename;
    int line;
    Type *type;
    bool is_lvalue;
    ExprNode *parent;
    char *identifier;
    int index;
};

struct StmtNode {
    int kind;
    char *filename;
    int line;
};

struct CompoundNode {
    int kind;
    char *filename;
    int line;
    StmtNode **stmts;
    int num_stmts;
};

struct ReturnNode {
    int kind;
    char *filename;
    int line;
    ExprNode *return_value;
};

struct IfNode {
    int kind;
    char *filename;
    int line;
    ExprNode *condition;
    StmtNode *then;
    StmtNode *else_;
};

struct SwitchNode {
    int kind;
    char *filename;
    int line;
    ExprNode *condition;
    ExprNode **case_values;
    StmtNode **cases;
    int num_cases;
    StmtNode *default_;
};

struct WhileNode {
    int kind;
    char *filename;
    int line;
    ExprNode *condition;
    StmtNode *body;
};

struct DoNode {
    int kind;
    char *filename;
    int line;
    StmtNode *body;
    ExprNode *condition;
};

struct ForNode {
    int kind;
    char *filename;
    int line;
    ExprNode *initialization;
    ExprNode *condition;
    ExprNode *continuation;
    StmtNode *body;
};

struct BreakNode {
    int kind;
    char *filename;
    int line;
};

struct ContinueNode {
    int kind;
    char *filename;
    int line;
};

struct DeclStmtNode {
    int kind;
    char *filename;
    int line;
    DeclNode *decl;
};

struct ExprStmtNode {
    int kind;
    char *filename;
    int line;
    ExprNode *expr;
};

struct DeclNode {
    int kind;
    char *filename;
    int line;
    Symbol *symbol;
};

struct TypedefNode {
    int kind;
    char *filename;
    int line;
    Symbol *symbol;
};

struct ExternNode {
    int kind;
    char *filename;
    int line;
    Symbol *symbol;
};

struct MemberNode {
    int kind;
    char *filename;
    int line;
    Symbol *symbol;
};

struct VariableNode {
    int kind;
    char *filename;
    int line;
    Symbol *symbol;
};

struct FunctionNode {
    int kind;
    char *filename;
    int line;
    Symbol *symbol;
    VariableNode **params;
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

typedef struct ScopeStack {
    Vec *scopes;
} ScopeStack;

ScopeStack *scope_stack_new(void);
int scope_stack_depth(ScopeStack *s);
void scope_stack_push(ScopeStack *s);
void scope_stack_pop(ScopeStack *s);
Symbol *scope_stack_find(ScopeStack *s, const char *name, bool recursive);
void scope_stack_register(ScopeStack *s, Symbol *symbol);

typedef struct ParserContext {
    ScopeStack *env;
    ScopeStack *struct_env;
    VariableSymbol *current_function;
    Vec *locals;
    Vec *flow_state;
    const Token **tokens;
    int index;
} ParserContext;

Type *parse_type(ParserContext *ctx);
ExprNode *parse_unary_expr(ParserContext *ctx);
ExprNode *parse_assign_expr(ParserContext *ctx);
ExprNode *parse_expr(ParserContext *ctx);
StmtNode *parse_stmt(ParserContext *ctx);

void parse_declarator_postfix(ParserContext *ctx, Type **type);
void parse_declarator(ParserContext *ctx, Type **type, const Token **t);
void parse_abstract_declarator(ParserContext *ctx, Type **type);
DeclNode *parse_decl(ParserContext *ctx);
VariableNode *parse_param(ParserContext *ctx);
DeclNode *parse_top_level(ParserContext *ctx);
TranslationUnitNode *parse(const char *filename, const char *src,
                           Vec *include_directories);

Type *sema_identifier_type(ParserContext *ctx, const Token *t);
MemberNode *sema_struct_member(ParserContext *ctx, Type *type, const Token *t);
Type *sema_struct_type_without_body(ParserContext *ctx, const Token *t,
                                    const Token *identifier);
StructType *sema_struct_type_enter(ParserContext *ctx, const Token *t,
                                   const Token *identifier);
Type *sema_struct_type_leave(ParserContext *ctx, StructType *type,
                             const Token *t, MemberNode **members,
                             int num_members);

ExprNode *sema_paren_expr(ParserContext *ctx, const Token *open, ExprNode *expr,
                          const Token *close);
ExprNode *sema_integer_expr(ParserContext *ctx, const Token *t, int value);
ExprNode *sema_string_expr(ParserContext *ctx, const Token *t,
                           const char *string, int length);
ExprNode *sema_identifier_expr(ParserContext *ctx, const Token *t);
ExprNode *sema_postfix_expr(ParserContext *ctx, ExprNode *operand,
                            const Token *t);
ExprNode *sema_call_expr(ParserContext *ctx, ExprNode *callee,
                         const Token *open, ExprNode **args, int num_args,
                         const Token *close);
ExprNode *sema_dot_expr(ParserContext *ctx, ExprNode *parent, const Token *t,
                        const Token *identifier);
ExprNode *sema_arrow_expr(ParserContext *ctx, ExprNode *parent, const Token *t,
                          const Token *identifier);
ExprNode *sema_unary_expr(ParserContext *ctx, const Token *t,
                          ExprNode *operand);
ExprNode *sema_sizeof_expr(ParserContext *ctx, const Token *t, Type *operand);
ExprNode *sema_cast_expr(ParserContext *ctx, const Token *open, Type *type,
                         const Token *close, ExprNode *operand);
ExprNode *sema_binary_expr(ParserContext *ctx, ExprNode *left, const Token *t,
                           ExprNode *right);

void sema_compound_stmt_enter(ParserContext *ctx);
StmtNode *sema_compound_stmt_leave(ParserContext *ctx, const Token *open,
                                   StmtNode **stmts, int num_stmts,
                                   const Token *close);
StmtNode *sema_return_stmt(ParserContext *ctx, const Token *t,
                           ExprNode *return_value);
void sema_if_stmt_enter_block(ParserContext *ctx);
void sema_if_stmt_leave_block(ParserContext *ctx);
StmtNode *sema_if_stmt(ParserContext *ctx, const Token *t, ExprNode *condition,
                       StmtNode *then, StmtNode *else_);
StmtNode *sema_switch_stmt_case(ParserContext *ctx, const Token *t,
                                ExprNode *case_value, StmtNode **stmts,
                                int num_stmts);
StmtNode *sema_switch_stmt_default(ParserContext *ctx, const Token *t,
                                   StmtNode **stmts, int num_stmts);
void sema_switch_stmt_enter(ParserContext *ctx);
StmtNode *sema_switch_stmt_leave(ParserContext *ctx, const Token *t,
                                 ExprNode *condition, ExprNode **case_values,
                                 StmtNode **cases, int num_cases,
                                 StmtNode *default_);
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

Type *sema_array_declarator(ParserContext *ctx, Type *type, ExprNode *size);

DeclNode *sema_typedef(ParserContext *ctx, const Token *t, Type *type,
                       const Token *identifier);
DeclNode *sema_extern(ParserContext *ctx, const Token *t, Type *type,
                      const Token *identifier);
DeclNode *sema_var_decl(ParserContext *ctx, Type *type,
                        const Token *identifier);
VariableNode *sema_param(ParserContext *ctx, Type *type,
                         const Token *identifier);

void sema_function_enter_params(ParserContext *ctx);
FunctionNode *sema_function_leave_params(ParserContext *ctx, Type *return_type,
                                         const Token *t, VariableNode **params,
                                         int num_params, bool var_args);
void sema_function_enter_body(ParserContext *ctx, FunctionNode *p);
FunctionNode *sema_function_leave_body(ParserContext *ctx, FunctionNode *p,
                                       StmtNode *body);

ParserContext *sema_translation_unit_enter(const Token **tokens);
TranslationUnitNode *sema_translation_unit_leave(ParserContext *ctx,
                                                 const char *filename,
                                                 DeclNode **decls,
                                                 int num_decls);

typedef struct GeneratorContext {
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    Vec *break_targets;
    Vec *continue_targets;
} GeneratorContext;

LLVMTypeRef generate_type(GeneratorContext *ctx, Type *p);
LLVMValueRef generate_expr(GeneratorContext *ctx, ExprNode *p);
LLVMValueRef generate_expr_addr(GeneratorContext *ctx, ExprNode *p);
bool generate_stmt(GeneratorContext *ctx, StmtNode *p);

LLVMValueRef generate_function(GeneratorContext *ctx, FunctionNode *p);
void generate_decl(GeneratorContext *ctx, DeclNode *p);
LLVMModuleRef generate(TranslationUnitNode *p);

#endif
