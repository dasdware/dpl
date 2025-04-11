#ifndef __DPL_PARSER_H
#define __DPL_PARSER_H

#include <dpl/lexer.h>
#include <dpl/symbols.h>

struct DPL_Ast_Type;

typedef struct
{
    DPL_Token name;
    DPL_Token first;
    DPL_Token last;
    struct DPL_Ast_Type *type;
} DPL_Ast_TypeField;

typedef struct {
    DPL_Ast_TypeField* items;
    size_t count;
    size_t capacity;
} DPL_Ast_TypeFields;

typedef struct
{
    size_t field_count;
    DPL_Ast_TypeField *fields;
} DPL_Ast_TypeObject;

typedef struct DPL_Ast_Type
{
    DPL_Symbol_Type_Kind kind;
    DPL_Token first;
    DPL_Token last;
    union
    {
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
    AST_NODE_FOR_LOOP,
    AST_NODE_FIELD_ACCESS,
    AST_NODE_INTERPOLATION,

    COUNT_AST_NODE_KINDS,
} DPL_AstNodeKind;

typedef struct DPL_Ast_Node DPL_Ast_Node;

typedef struct
{
    DPL_Token value;
} DPL_Ast_Literal;

typedef struct
{
    size_t field_count;
    DPL_Ast_Node **fields;
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

typedef struct parser
{
    DPL_Token variable_name;
    DPL_Ast_Node *iterator_initializer;
    DPL_Ast_Node *body;
} DPL_Ast_ForLoop;

typedef struct
{
    DPL_Token name;

    size_t argument_count;
    DPL_Ast_Node **arguments;
} DPL_Ast_FunctionCall;

typedef struct
{
    size_t expression_count;
    DPL_Ast_Node **expressions;
} DPL_Ast_Scope;

typedef struct
{
    size_t expression_count;
    DPL_Ast_Node **expressions;
} DPL_Ast_Interpolation;

typedef struct
{
    DPL_Token keyword;
    DPL_Token name;
    DPL_Ast_Type *type;
    DPL_Token assignment;
    DPL_Ast_Node *initialization;
} DPL_Ast_Declaration;

typedef struct
{
    DPL_Ast_Node *target;
    DPL_Token assignment;
    DPL_Ast_Node *expression;
} DPL_Ast_Assignment;

typedef struct
{
    DPL_Token name;
    DPL_Ast_Type *type;
} DPL_Ast_FunctionArgument;

typedef struct
{
    size_t argument_count;
    DPL_Ast_FunctionArgument *arguments;
    DPL_Ast_Type *type;
} DPL_Ast_FunctionSignature;

typedef struct
{
    DPL_Token keyword;
    DPL_Token name;
    DPL_Ast_FunctionSignature signature;
    DPL_Ast_Node *body;
} DPL_Ast_Function;

typedef struct
{
    DPL_Ast_Node *expression;
    DPL_Ast_Node *field;
} DPL_Ast_FieldAccess;

struct DPL_Ast_Node
{
    DPL_AstNodeKind kind;
    DPL_Token first;
    DPL_Token last;
    union
    {
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
        DPL_Ast_ForLoop for_loop;
        DPL_Ast_FieldAccess field_access;
        DPL_Ast_Interpolation interpolation;
    } as;
};

typedef struct {
    DPL_Ast_Node **items;
    size_t count;
    size_t capacity;
} DPL_Ast_Nodes;

typedef struct
{
    Arena *memory;
    DPL_Lexer *lexer;
} DPL_Parser;

#define DPL_AST_ERROR(source, node, format, ...)                                             \
    do                                                                                       \
    {                                                                                        \
        DW_ERROR_MSG(LOC_Fmt ": ERROR: ", LOC_Arg((node)->first.location));                  \
        DW_ERROR_MSG(format, ##__VA_ARGS__);                                                 \
        DW_ERROR_MSG("\n");                                                                  \
        dpl_lexer_print_token_range((source), DW_ERROR_STREAM, (node)->first, (node)->last); \
        exit(1);                                                                             \
    } while (false)

#define DPL_AST_ERROR_WITH_NOTE(source, note_node, note, error_node, error_format, ...)                  \
    do                                                                                                   \
    {                                                                                                    \
        DW_ERROR_MSG(LOC_Fmt ": ERROR: ", LOC_Arg((error_node)->first.location));                        \
        DW_ERROR_MSG(error_format, ##__VA_ARGS__);                                                       \
        DW_ERROR_MSG("\n");                                                                              \
        dpl_lexer_print_token_range((source), DW_ERROR_STREAM, (error_node)->first, (error_node)->last); \
        DW_ERROR_MSG(LOC_Fmt ": NOTE: " note, LOC_Arg((note_node)->first.location));                     \
        DW_ERROR_MSG("\n");                                                                              \
        dpl_lexer_print_token_range((source), DW_ERROR_STREAM, (note_node)->first, (note_node)->last);   \
        exit(1);                                                                                         \
    } while (false)

const char *dpl_parse_nodekind_name(DPL_AstNodeKind kind);
void dpl_parse_print(DPL_Ast_Node *node, size_t level);

DPL_Ast_Node *dpl_parse(DPL_Parser *parser);

#endif