#ifndef __DPL_VM_H
#define __DPL_VM_H

#include "arena.h"
#include "externals.h"
#include "value.h"
#include "dw_memory_table.h"

#include <dpl/program.h>

typedef struct
{
    size_t stack_top;
    size_t arity;
    size_t return_ip;
} DPL_CallFrame;

typedef struct DPL_VirtualMachine
{
    DPL_Program *program;
    DPL_ExternalFunctions externals;

    bool debug;
    bool trace;

    size_t stack_capacity;
    size_t stack_top;
    DPL_Value *stack;
    DW_MemoryTable stack_memory;

    size_t callstack_capacity;
    size_t callstack_top;
    DPL_CallFrame *callstack;

    Arena memory;
} DPL_VirtualMachine;

void dplv_init(DPL_VirtualMachine *vm, DPL_Program *program, DPL_ExternalFunctions externals);
void dplv_free(DPL_VirtualMachine *vm);

void dplv_run(DPL_VirtualMachine *vm);

DPL_Value dplv_peek(DPL_VirtualMachine *vm);

DPL_Value dplv_reference(DPL_VirtualMachine *vm, DPL_Value value);
void dplv_release(DPL_VirtualMachine *vm, DPL_Value value);

void dplv_return(DPL_VirtualMachine *vm, size_t arity, DPL_Value value);
void dplv_return_number(DPL_VirtualMachine *vm, size_t arity, double value);
void dplv_return_string(DPL_VirtualMachine *vm, size_t arity, Nob_String_View value);
void dplv_return_boolean(DPL_VirtualMachine *vm, size_t arity, bool value);

#endif // __DPL_VM_H