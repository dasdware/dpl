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

typedef struct
{
    // Configuration
    bool debug;
    Nob_String_View file_name;
    Nob_String_View source;

    // Symbol stack
    DPL_SymbolStack symbols;

    Arena *memory;
} DPL;

void dpl_init(DPL *dpl, DPL_ExternalFunctions externals);
void dpl_free(DPL *dpl);

void dpl_compile(DPL *dpl, DPL_Program *program);

#endif // __DPL_H
