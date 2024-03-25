#ifndef __DPL_CALLTREE_H
#define __DPL_CALLTREE_H

#include "lexer.h"
#include "parser.h"
#include "functions.h"
#include "types.h"
#include "arena.h"

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
    // dependencies
    DPL_Types *types;
    DPL_Functions *functions;
    DPL_Lexer *lexer;
    DPL_Parser *parser;

    Arena memory;
    DPL_CallTree_Node *root;
} DPL_CallTree;

void dplc_init(DPL_CallTree *call_tree, DPL_Types *types, DPL_Functions *functions,
               DPL_Lexer *lexer, DPL_Parser *parser);
void dplc_free(DPL_CallTree *call_tree);

void dplc_bind(DPL_CallTree *call_tree);

void dplc_print(DPL_CallTree* tree, DPL_CallTree_Node* node, size_t level);

#endif // __DPL_CALLTREE_H