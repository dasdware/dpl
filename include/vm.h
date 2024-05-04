#ifndef __DPL_VM_H
#define __DPL_VM_H

#include "arena.h"
#include "program.h"
#include "value.h"
#include "string_table.h"

struct DPL_ExternalFunctions;

typedef struct
{
    DPL_Program *program;
    struct DPL_ExternalFunctions *externals;

    bool debug;
    bool trace;

    size_t stack_capacity;
    size_t stack_top;
    DPL_Value *stack;

    DW_StringTable strings;

    Arena memory;
} DPL_VirtualMachine;

void dplv_init(DPL_VirtualMachine *vm, DPL_Program *program, struct DPL_ExternalFunctions *externals);
void dplv_free(DPL_VirtualMachine *vm);

void dplv_run(DPL_VirtualMachine *vm);

DPL_Value dplv_peek(DPL_VirtualMachine *vm);
void dplv_replace_top(DPL_VirtualMachine *vm, DPL_Value value);

#endif // __DPL_VM_H