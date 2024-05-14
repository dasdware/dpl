#ifndef __DPL_VM_H
#define __DPL_VM_H

#include "arena.h"
#include "program.h"
#include "value.h"
#include "string_table.h"

struct DPL_ExternalFunctions;

typedef struct {
    size_t stack_top;
    size_t return_ip;
} DPL_CallFrame;

typedef struct
{
    DPL_Program *program;
    struct DPL_ExternalFunctions *externals;

    bool debug;
    bool trace;

    size_t stack_capacity;
    size_t stack_top;
    DPL_Value *stack;

    size_t callstack_capacity;
    size_t callstack_top;
    DPL_CallFrame *callstack;

    DW_StringTable strings;

    Arena memory;
} DPL_VirtualMachine;

void dplv_init(DPL_VirtualMachine *vm, DPL_Program *program, struct DPL_ExternalFunctions *externals);
void dplv_free(DPL_VirtualMachine *vm);

void dplv_run(DPL_VirtualMachine *vm);

DPL_Value dplv_peek(DPL_VirtualMachine *vm);

DPL_Value dplv_reference(DPL_VirtualMachine* vm, DPL_Value value);
void dplv_release(DPL_VirtualMachine* vm, DPL_Value value);

void dplv_return(DPL_VirtualMachine* vm, size_t arity, DPL_Value value);
void dplv_return_number(DPL_VirtualMachine* vm, size_t arity, double value);
void dplv_return_string(DPL_VirtualMachine* vm, size_t arity, Nob_String_View value);

#endif // __DPL_VM_H