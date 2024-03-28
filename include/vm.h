#ifndef __DPL_VM_H
#define __DPL_VM_H

#include "arena.h"
#include "program.h"

typedef double DPL_Value;

typedef struct
{
    DPL_ByteCode *program;
    bool debug;

    size_t stack_capacity;
    size_t stack_top;
    DPL_Value *stack;

    Arena memory;
} DPL_VirtualMachine;

void dplv_init(DPL_VirtualMachine *vm, DPL_ByteCode *program);
void dplv_free(DPL_VirtualMachine *vm);

void dplv_run(DPL_VirtualMachine *vm);

#endif // __DPL_VM_H