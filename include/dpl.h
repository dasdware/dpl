#ifndef __DPL_H
#define __DPL_H

#include "arena.h"
#include "dw_array.h"
#include "nob.h"

#include "externals.h"
#include "program.h"

// COMMON

typedef struct _DPL DPL;

typedef uint16_t DPL_Handle;
typedef da_array(DPL_Handle) DPL_Handles;

typedef struct {
    DPL_Handles arguments;
    DPL_Handle returns;
} DPL_Signature;

// CATALOG

/// TYPES

typedef union
{
    void* base;
    DPL_Signature function;
} DPL_Type_As;

typedef enum
{
    TYPE_BASE,
    TYPE_FUNCTION,
} DPL_Type_Kind;

typedef struct
{
    Nob_String_View name;
    DPL_Handle handle;
    size_t hash;

    DPL_Type_Kind kind;
    DPL_Type_As as;
} DPL_Type;

typedef struct
{
    DPL_Type *items;
    size_t count;
    size_t capacity;
} DPL_Types;

/// FUNCTIONS

typedef void (*DPL_Generator_Callback)(DPL* dpl, DPL_Program *, void *);

typedef struct
{
    DPL_Generator_Callback callback;
    void *user_data;
} DPL_Generator;

typedef struct
{
    DPL_Handle handle;
    Nob_String_View name;
    DPL_Signature signature;
    DPL_Generator generator;
} DPL_Function;

typedef da_array(DPL_Function) DPL_Functions;

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

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,

    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_COLON_EQUAL,

    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,

    TOKEN_KEYWORD_CONSTANT,
    TOKEN_KEYWORD_FUNCTION,
    TOKEN_KEYWORD_VAR,
} DPL_TokenKind;

typedef struct
{
    DPL_TokenKind kind;
    DPL_Location location;
    Nob_String_View text;
} DPL_Token;

// PARSER

typedef enum
{
    AST_NODE_LITERAL = 0,
    AST_NODE_UNARY,
    AST_NODE_BINARY,
    AST_NODE_FUNCTIONCALL,
    AST_NODE_SCOPE,
    AST_NODE_DECLARATION,
    AST_NODE_SYMBOL,
    AST_NODE_ASSIGNMENT,
    AST_NODE_FUNCTION,
} DPL_AstNodeKind;

typedef struct _DPL_Ast_Node DPL_Ast_Node;

typedef struct
{
    DPL_Token value;
} DPL_Ast_Literal;

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
    DPL_Token keyword;
    DPL_Token name;
    DPL_Token type;
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
    DPL_Token type_name;
} DPL_Ast_FunctionArgument;

typedef struct {
    size_t argument_count;
    DPL_Ast_FunctionArgument* arguments;
    DPL_Token type_name;
} DPL_Ast_FunctionSignature;

typedef struct {
    DPL_Token keyword;
    DPL_Token name;
    DPL_Ast_FunctionSignature signature;
    DPL_Ast_Node* body;
} DPL_Ast_Function;

union DPL_Ast_Node_As
{
    DPL_Ast_Literal literal;
    DPL_Ast_Unary unary;
    DPL_Ast_Binary binary;
    DPL_Ast_FunctionCall function_call;
    DPL_Ast_Scope scope;
    DPL_Ast_Declaration declaration;
    DPL_Token symbol;
    DPL_Ast_Assignment assignment;
    DPL_Ast_Function function;
};

struct _DPL_Ast_Node
{
    DPL_AstNodeKind kind;
    DPL_Token first;
    DPL_Token last;
    union DPL_Ast_Node_As as;
};

typedef struct
{
    Arena memory;
    DPL_Ast_Node *root;
} DPL_Ast_Tree;


// CALLTREE

typedef struct _DPL_CallTree_Node DPL_CallTree_Node;

typedef union {
    double number;
    Nob_String_View string;
} DPL_CallTree_Value_As;

typedef struct {
    DPL_Handle type_handle;
    DPL_CallTree_Value_As as;
} DPL_CallTree_Value;

typedef enum {
    SYMBOL_CONSTANT,
    SYMBOL_VAR,
    SYMBOL_FUNCTION,
    SYMBOL_ARGUMENT,
} DPL_SymbolKind;

typedef struct {
    DPL_Handle type_handle;
    size_t scope_index;
} DPL_CallTree_Var;

typedef struct {
    DPL_Signature signature;
    DPL_CallTree_Node* body;
    bool used;
    DPL_Handle user_handle;
    DPL_Handle function_handle;
} DPL_CallTree_Function;

typedef union {
    DPL_CallTree_Value constant;
    DPL_CallTree_Var var;
    DPL_CallTree_Var argument;
    DPL_CallTree_Function function;
} DPL_Symbol_As;

typedef struct {
    DPL_SymbolKind kind;
    Nob_String_View name;
    DPL_Symbol_As as;
} DPL_Symbol;

typedef struct {
    DPL_Symbol *items;
    size_t count;
    size_t capacity;
} DPL_Symbols;

typedef struct {
    size_t *items;
    size_t count;
    size_t capacity;
} DPL_Frames;

typedef struct {
    DPL_Symbols symbols;
    DPL_Frames frames;
    size_t bottom;
} DPL_SymbolStack;

typedef struct {
    size_t offset;
    size_t count;
} DPL_Scope;

typedef struct {
    DPL_Scope *items;
    size_t count;
    size_t capacity;
} DPL_ScopeStack;

typedef enum
{
    CALLTREE_NODE_VALUE = 0,
    CALLTREE_NODE_FUNCTIONCALL,
    CALLTREE_NODE_SCOPE,
    CALLTREE_NODE_VARREF,
    CALLTREE_NODE_ARGREF,
    CALLTREE_NODE_ASSIGNMENT,
} DPL_CallTreeNodeKind;

typedef struct
{
    DPL_CallTree_Node **items;
    size_t count;
    size_t capacity;
} DPL_CallTree_Nodes;

typedef struct
{
    DPL_Handle function_handle;
    DPL_CallTree_Nodes arguments;
} DPL_CallTree_FunctionCall;

typedef struct
{
    DPL_CallTree_Nodes expressions;
} DPL_CallTree_Scope;

typedef struct {
    size_t scope_index;
    DPL_CallTree_Node* expression;
} DPL_CallTree_Assignment;

typedef union
{
    DPL_CallTree_Value value;
    DPL_CallTree_FunctionCall function_call;
    DPL_CallTree_Scope scope;
    size_t varref;
    size_t argref;
    DPL_CallTree_Assignment assignment;
} DPL_CallTree_Node_As;

struct _DPL_CallTree_Node
{
    DPL_CallTreeNodeKind kind;
    DPL_Handle type_handle;
    bool persistent;
    DPL_CallTree_Node_As as;
};

typedef struct
{
    Arena memory;
    DPL_CallTree_Node *root;
} DPL_CallTree;

typedef struct {
    DPL_Handle function_handle;
    size_t begin_ip;
    size_t arity;
    DPL_CallTree_Node* body;
} DPL_UserFunction;

typedef struct {
    DPL_UserFunction* items;
    size_t count;
    size_t capacity;
} DPL_UserFunctions;

// COMPILATION CONTEXT

struct _DPL
{
    // Configuration
    bool debug;

    // Catalogs
    DPL_Types types;
    DPL_Handle number_type_handle;
    DPL_Handle string_type_handle;

    DPL_Functions functions;

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

    // Parser
    DPL_Ast_Tree tree;

    // Binder
    DPL_SymbolStack symbol_stack;
    DPL_ScopeStack scope_stack;
    DPL_CallTree calltree;

    // Generator
    DPL_UserFunctions user_functions;
};

void dpl_init(DPL *dpl, DPL_ExternalFunctions* externals);
void dpl_free(DPL *dpl);

void dpl_compile(DPL *dpl, DPL_Program *program);

#endif // __DPL_H
