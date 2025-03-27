#ifndef __DPL_H
#define __DPL_H

#include "arena.h"
#include "dw_array.h"
#include "nob.h"

#include <dpl/binding.h>
#include <dpl/lexer.h>
#include <dpl/parser.h>
#include <dpl/symbols.h>

#include "externals.h"
#include "program.h"

// COMMON

typedef struct _DPL DPL;

#ifndef DPL_MAX_INTERPOLATION
#define DPL_MAX_INTERPOLATION 8
#endif

typedef struct
{
    Arena memory;
    DPL_Ast_Node *root;
} DPL_Ast_Tree;

// BOUND TREE

typedef struct
{
    Arena memory;
    DPL_Bound_Node *root;
} DPL_BoundTree;

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
};

void dpl_init(DPL *dpl, DPL_ExternalFunctions externals);
void dpl_free(DPL *dpl);

void dpl_compile(DPL *dpl, DPL_Program *program);

#endif // __DPL_H
