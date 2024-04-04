#ifndef __DPL_H
#define __DPL_H

#include "arena.h"
#include "nob.h"

#include "program.h"

// CATALOG

/// TYPES

typedef size_t DPL_Type_Handle;

typedef struct
{
    DPL_Type_Handle *items;
    size_t count;
    size_t capacity;
} DPL_Type_Handles;

typedef struct
{
} DPL_Type_AsBase;

typedef struct
{
    DPL_Type_Handles arguments;
    DPL_Type_Handle returns;
} DPL_Type_AsFunction;

typedef union
{
    DPL_Type_AsBase base;
    DPL_Type_AsFunction function;
} DPL_Type_As;

typedef enum
{
    TYPE_BASE,
    TYPE_FUNCTION,
} DPL_Type_Kind;

typedef struct
{
    Nob_String_View name;
    DPL_Type_Handle handle;
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
    DPL_Type_Handle number_handle;

    // Common function types
    DPL_Type_Handle unary_handle;
    DPL_Type_Handle binary_handle;
} DPL_Types;

/// FUNCTIONS

typedef size_t DPL_Function_Handle;

typedef struct
{
    DPL_Function_Handle handle;
    Nob_String_View name;
    DPL_Type_Handle type;
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

    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,

    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
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

union DPL_Ast_Node_As
{
    DPL_Ast_Literal literal;
    DPL_Ast_Unary unary;
    DPL_Ast_Binary binary;
};

struct _DPL_Ast_Node
{
    DPL_AstNodeKind kind;
    union DPL_Ast_Node_As as;
};

typedef struct
{
    Arena memory;
    DPL_Ast_Node *root;
} DPL_Ast_Tree;

// CALLTREE


typedef enum
{
    CALLTREE_NODE_VALUE = 0,
    CALLTREE_NODE_FUNCTION
} DPL_CallTreeNodeKind;

typedef struct _DPL_CallTree_Node DPL_CallTree_Node;

typedef struct
{
    DPL_Ast_Node *ast_node;
} DPL_CallTree_Value;

typedef struct
{
    DPL_CallTree_Node **items;
    size_t count;
    size_t capacity;
} DPL_CallTree_Nodes;

typedef struct
{
    DPL_Function_Handle function_handle;
    DPL_CallTree_Nodes arguments;
} DPL_CallTree_Function;

typedef union
{
    DPL_CallTree_Value value;
    DPL_CallTree_Function function;
} DPL_CallTree_Node_As;

struct _DPL_CallTree_Node
{
    DPL_CallTreeNodeKind kind;
    DPL_Type_Handle type_handle;
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
    DPL_Token peek_token;

    // Parser
    DPL_Ast_Tree tree;

    // Binder
    DPL_CallTree calltree;
} DPL;

void dpl_init(DPL *dpl);
void dpl_free(DPL *dpl);

void dpl_compile(DPL *dpl, DPL_Program *program);

#endif // __DPL_H
