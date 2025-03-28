#ifndef __DPL_H
#define __DPL_H

#include "arena.h"
#include "dw_array.h"
#include "nob.h"

#include <dpl/symbols.h>
#include "externals.h"
#include "program.h"

// COMMON

typedef struct _DPL DPL;

#ifndef DPL_MAX_INTERPOLATION
#define DPL_MAX_INTERPOLATION 8
#endif

// LOCATION

typedef struct
{
    Nob_String_View file_name;

    size_t line;
    size_t column;

    const char *line_start;
} DPL_Location;

#define LOC_Fmt SV_Fmt ":%zu:%zu"
#define LOC_Arg(loc) SV_Arg(loc.file_name), (loc.line + 1), (loc.column + 1)

// LEXER

typedef enum
{
    TOKEN_NONE = 0,
    TOKEN_EOF,
    TOKEN_WHITESPACE,
    TOKEN_COMMENT,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,

    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_AND_AND,
    TOKEN_PIPE_PIPE,

    TOKEN_DOT,
    TOKEN_DOT_DOT,
    TOKEN_COLON,
    TOKEN_COLON_EQUAL,

    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_OPEN_BRACKET,
    TOKEN_CLOSE_BRACKET,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_STRING_INTERPOLATION,
    TOKEN_TRUE,
    TOKEN_FALSE,

    TOKEN_KEYWORD_CONSTANT,
    TOKEN_KEYWORD_FUNCTION,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_TYPE,
} DPL_TokenKind;

typedef struct
{
    DPL_TokenKind kind;
    DPL_Location location;
    Nob_String_View text;
} DPL_Token;

// PARSER

struct DPL_Ast_Type;

typedef struct {
    DPL_Token name;
    DPL_Token first;
    DPL_Token last;
    struct DPL_Ast_Type* type;
} DPL_Ast_TypeField;

typedef struct {
    size_t field_count;
    DPL_Ast_TypeField* fields;
} DPL_Ast_TypeObject;

typedef struct DPL_Ast_Type {
    DPL_Symbol_Type_Kind kind;
    DPL_Token first;
    DPL_Token last;
    union {
        DPL_Token name;
        DPL_Ast_TypeObject object;
    } as;
} DPL_Ast_Type;

typedef enum
{
    AST_NODE_LITERAL = 0,
    AST_NODE_OBJECT_LITERAL,
    AST_NODE_UNARY,
    AST_NODE_BINARY,
    AST_NODE_FUNCTIONCALL,
    AST_NODE_SCOPE,
    AST_NODE_DECLARATION,
    AST_NODE_SYMBOL,
    AST_NODE_ASSIGNMENT,
    AST_NODE_FUNCTION,
    AST_NODE_CONDITIONAL,
    AST_NODE_WHILE_LOOP,
    AST_NODE_FIELD_ACCESS,
    AST_NODE_INTERPOLATION,
} DPL_AstNodeKind;

typedef struct _DPL_Ast_Node DPL_Ast_Node;

typedef struct
{
    DPL_Token value;
} DPL_Ast_Literal;

typedef struct {
    size_t field_count;
    DPL_Ast_Node** fields;
} DPL_Ast_ObjectLiteral;

typedef struct
{
    DPL_Token operator;
    DPL_Ast_Node *operand;
} DPL_Ast_Unary;

typedef struct
{
    DPL_Token operator;
    DPL_Ast_Node *left;
    DPL_Ast_Node *right;
} DPL_Ast_Binary;

typedef struct
{
    DPL_Ast_Node *condition;
    DPL_Ast_Node *then_clause;
    DPL_Ast_Node *else_clause;
} DPL_Ast_Conditional;

typedef struct
{
    DPL_Ast_Node *condition;
    DPL_Ast_Node *body;
} DPL_Ast_WhileLoop;

typedef struct
{
    DPL_Token name;

    size_t argument_count;
    DPL_Ast_Node** arguments;
} DPL_Ast_FunctionCall;

typedef struct
{
    size_t expression_count;
    DPL_Ast_Node** expressions;
} DPL_Ast_Scope;

typedef struct
{
    size_t expression_count;
    DPL_Ast_Node** expressions;
} DPL_Ast_Interpolation;

typedef struct
{
    DPL_Token keyword;
    DPL_Token name;
    DPL_Ast_Type* type;
    DPL_Token assignment;
    DPL_Ast_Node* initialization;
} DPL_Ast_Declaration;

typedef struct
{
    DPL_Ast_Node* target;
    DPL_Token assignment;
    DPL_Ast_Node* expression;
} DPL_Ast_Assignment;

typedef struct {
    DPL_Token name;
    DPL_Ast_Type* type;
} DPL_Ast_FunctionArgument;

typedef struct {
    size_t argument_count;
    DPL_Ast_FunctionArgument* arguments;
    DPL_Ast_Type* type;
} DPL_Ast_FunctionSignature;

typedef struct {
    DPL_Token keyword;
    DPL_Token name;
    DPL_Ast_FunctionSignature signature;
    DPL_Ast_Node* body;
} DPL_Ast_Function;

typedef struct {
    DPL_Ast_Node* expression;
    DPL_Ast_Node* field;
} DPL_Ast_FieldAccess;

struct _DPL_Ast_Node
{
    DPL_AstNodeKind kind;
    DPL_Token first;
    DPL_Token last;
    union {
        DPL_Ast_Literal literal;
        DPL_Ast_ObjectLiteral object_literal;
        DPL_Ast_Unary unary;
        DPL_Ast_Binary binary;
        DPL_Ast_FunctionCall function_call;
        DPL_Ast_Scope scope;
        DPL_Ast_Declaration declaration;
        DPL_Token symbol;
        DPL_Ast_Assignment assignment;
        DPL_Ast_Function function;
        DPL_Ast_Conditional conditional;
        DPL_Ast_WhileLoop while_loop;
        DPL_Ast_FieldAccess field_access;
        DPL_Ast_Interpolation interpolation;
    } as;
};

typedef struct
{
    Arena memory;
    DPL_Ast_Node *root;
} DPL_Ast_Tree;


// BOUND TREE

typedef enum
{
    BOUND_NODE_VALUE,
    BOUND_NODE_OBJECT,
    BOUND_NODE_FUNCTIONCALL,
    BOUND_NODE_SCOPE,
    BOUND_NODE_VARREF,
    BOUND_NODE_ARGREF,
    BOUND_NODE_ASSIGNMENT,
    BOUND_NODE_CONDITIONAL,
    BOUND_NODE_LOGICAL_OPERATOR,
    BOUND_NODE_WHILE_LOOP,
    BOUND_NODE_LOAD_FIELD,
    BOUND_NODE_INTERPOLATION,
} DPL_BoundNodeKind;

typedef struct _DPL_Bound_Node DPL_Bound_Node;

typedef struct {
    Nob_String_View name;
    DPL_Bound_Node* expression;
} DPL_Bound_ObjectField;

typedef struct {
    size_t field_count;
    DPL_Bound_ObjectField* fields;
} DPL_Bound_Object;

typedef struct
{
    DPL_Symbol* function;
    DPL_Bound_Node** arguments;
    size_t arguments_count;
} DPL_Bound_FunctionCall;

typedef struct
{
    DPL_Bound_Node** expressions;
    size_t expressions_count;
} DPL_Bound_Scope;

typedef struct {
    size_t scope_index;
    DPL_Bound_Node* expression;
} DPL_Bound_Assignment;

typedef struct {
    DPL_Bound_Node* condition;
    DPL_Bound_Node* then_clause;
    DPL_Bound_Node* else_clause;
} DPL_Bound_Conditional;

typedef struct {
    DPL_Token operator;
    DPL_Bound_Node *lhs;
    DPL_Bound_Node *rhs;
} DPL_Bound_LogicalOperator;

typedef struct {
    DPL_Bound_Node* condition;
    DPL_Bound_Node* body;
} DPL_Bound_WhileLoop;

typedef struct {
    DPL_Bound_Node* expression;
    size_t field_index;
} DPL_Bound_LoadField;

typedef struct
{
    DPL_Bound_Node** expressions;
    size_t expressions_count;
} DPL_Bound_Interpolation;

struct _DPL_Bound_Node
{
    DPL_BoundNodeKind kind;
    DPL_Symbol* type;
    bool persistent;
    union {
        DPL_Symbol_Constant value;
        DPL_Bound_Object object;
        DPL_Bound_FunctionCall function_call;
        DPL_Bound_Scope scope;
        size_t varref;
        size_t argref;
        DPL_Bound_Assignment assignment;
        DPL_Bound_Conditional conditional;
        DPL_Bound_LogicalOperator logical_operator;
        DPL_Bound_WhileLoop while_loop;
        DPL_Bound_LoadField load_field;
        DPL_Bound_Interpolation interpolation;
    } as;
};

typedef struct
{
    Arena memory;
    DPL_Bound_Node *root;
} DPL_BoundTree;


// USER FUNCTIONS

typedef struct {
    DPL_Symbol* function;
    size_t begin_ip;
    size_t arity;
    DPL_Bound_Node* body;
} DPL_UserFunction;

typedef da_array(DPL_UserFunction) DPL_UserFunctions;

// COMPILATION CONTEXT

struct _DPL
{
    // Configuration
    bool debug;

    // Symbol stack
    DPL_SymbolStack symbols;

    // Common
    Nob_String_View file_name;
    Nob_String_View source;

    // Lexer
    size_t position;
    size_t token_start;
    DPL_Location token_start_location;
    const char *current_line;
    size_t line;
    size_t column;
    DPL_Token first_token;
    DPL_Token peek_token;
    int interpolation_brackets[DPL_MAX_INTERPOLATION];
    int interpolation_depth;

    // Parser
    DPL_Ast_Tree tree;

    // Binder
    DPL_BoundTree bound_tree;

    // Generator
    DPL_UserFunctions user_functions;
};

void dpl_init(DPL *dpl, DPL_ExternalFunctions externals);
void dpl_free(DPL *dpl);

void dpl_compile(DPL *dpl, DPL_Program *program);

#endif // __DPL_H
