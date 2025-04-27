#ifndef __DPL_H
#define __DPL_H

#include <arena.h>
#include <nobx.h>

#include <dpl/binding.h>
#include <dpl/lexer.h>
#include <dpl/parser.h>
#include <dpl/program.h>
#include <dpl/symbols.h>

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

void dpl_init(DPL *dpl);
void dpl_free(DPL *dpl);

void dpl_compile(DPL *dpl, DPL_Program *program);

#endif // __DPL_H
