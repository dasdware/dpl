#ifndef __DPL_AST_H
#define __DPL_AST_H

#include "arena.h"
#include "lexer.h"

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

void dpla_init(DPL_Ast_Tree *tree);
void dpla_free(DPL_Ast_Tree *tree);
void dpla_print(DPL_Ast_Node *node, size_t level);

const char* dpla_node_kind_name(DPL_AstNodeKind kind);

DPL_Ast_Node *dpla_create_node(DPL_Ast_Tree *tree, DPL_AstNodeKind kind);

#endif // __DPL_AST_H