#ifndef __DPL_H
#define __DPL_H

#include "arena.h"
#include "nob.h"

#include "externals.h"
#include "program.h"

// COMMON

#ifndef DPL_HANDLES_CAPACITY
#define DPL_HANDLES_CAPACITY 16
#endif

typedef uint16_t DPL_Handle;

typedef struct {
    DPL_Handle items[DPL_HANDLES_CAPACITY];
    uint8_t count;
} DPL_Handles;

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

    // Common datatypes
    DPL_Handle number_handle;
    DPL_Handle string_handle;
} DPL_Types;

/// FUNCTIONS

typedef void (*DPL_Generator_Callback)(DPL_Program *, void *);

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

typedef struct
{
    DPL_Function *items;
    size_t count;
    size_t capacity;
} DPL_Functions;

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

union DPL_Ast_Node_As
{
    DPL_Ast_Literal literal;
    DPL_Ast_Unary unary;
    DPL_Ast_Binary binary;
    DPL_Ast_FunctionCall function_call;
    DPL_Ast_Scope scope;
    DPL_Ast_Declaration declaration;
    DPL_Token symbol;
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
} DPL_SymbolKind;

typedef union {
    DPL_CallTree_Value constant;
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
} DPL_SymbolStack;

typedef enum
{
    CALLTREE_NODE_VALUE = 0,
    CALLTREE_NODE_FUNCTION,
    CALLTREE_NODE_SCOPE,
} DPL_CallTreeNodeKind;

typedef struct _DPL_CallTree_Node DPL_CallTree_Node;

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
} DPL_CallTree_Function;

typedef struct
{
    DPL_CallTree_Nodes expressions;
} DPL_CallTree_Scope;

typedef union
{
    DPL_CallTree_Value value;
    DPL_CallTree_Function function;
    DPL_CallTree_Scope scope;
} DPL_CallTree_Node_As;

struct _DPL_CallTree_Node
{
    DPL_CallTreeNodeKind kind;
    DPL_Handle type_handle;
    DPL_CallTree_Node_As as;
};

typedef struct
{
    Arena memory;
    DPL_CallTree_Node *root;
} DPL_CallTree;


// COMPILATION CONTEXT

typedef struct _DPL
{
    // Configuration
    bool debug;

    // Catalogs
    DPL_Types types;
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
    DPL_CallTree calltree;
} DPL;

void dpl_init(DPL *dpl, DPL_ExternalFunctions* externals);
void dpl_free(DPL *dpl);

void dpl_compile(DPL *dpl, DPL_Program *program);

#endif // __DPL_H
