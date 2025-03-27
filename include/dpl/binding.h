#ifndef __DPL_BINDING_H
#define __DPL_BINDING_H

#include <nob.h>

#include <dpl/lexer.h>
#include <dpl/parser.h>
#include <dpl/symbols.h>

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

    COUNT_BOUND_NODE_KINDS,
} DPL_BoundNodeKind;

typedef struct DPL_Bound_Node DPL_Bound_Node;

typedef struct
{
    Nob_String_View name;
    DPL_Bound_Node *expression;
} DPL_Bound_ObjectField;

typedef struct
{
    size_t field_count;
    DPL_Bound_ObjectField *fields;
} DPL_Bound_Object;

typedef struct
{
    DPL_Symbol *function;
    DPL_Bound_Node **arguments;
    size_t arguments_count;
} DPL_Bound_FunctionCall;

typedef struct
{
    DPL_Bound_Node **expressions;
    size_t expressions_count;
} DPL_Bound_Scope;

typedef struct
{
    size_t scope_index;
    DPL_Bound_Node *expression;
} DPL_Bound_Assignment;

typedef struct
{
    DPL_Bound_Node *condition;
    DPL_Bound_Node *then_clause;
    DPL_Bound_Node *else_clause;
} DPL_Bound_Conditional;

typedef struct
{
    DPL_Token operator;
    DPL_Bound_Node *lhs;
    DPL_Bound_Node *rhs;
} DPL_Bound_LogicalOperator;

typedef struct
{
    DPL_Bound_Node *condition;
    DPL_Bound_Node *body;
} DPL_Bound_WhileLoop;

typedef struct
{
    DPL_Bound_Node *expression;
    size_t field_index;
} DPL_Bound_LoadField;

typedef struct
{
    DPL_Bound_Node **expressions;
    size_t expressions_count;
} DPL_Bound_Interpolation;

struct DPL_Bound_Node
{
    DPL_BoundNodeKind kind;
    DPL_Symbol *type;
    bool persistent;
    union
    {
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
    DPL_Symbol *function;
    size_t begin_ip;
    size_t arity;
    DPL_Bound_Node *body;
} DPL_Binding_UserFunction;

typedef struct
{
    Arena *memory;
    Nob_String_View source;
    DPL_SymbolStack *symbols;
    da_array(DPL_Binding_UserFunction) user_functions;
} DPL_Binding;

const char *dpl_bind_nodekind_name(DPL_BoundNodeKind kind);
DPL_Bound_Node *dpl_bind_node(DPL_Binding *binding, DPL_Ast_Node *node);

void dpl_bind_print(DPL_Binding *binding, DPL_Bound_Node *node, size_t level);

#endif // __DPL_BINDING_H