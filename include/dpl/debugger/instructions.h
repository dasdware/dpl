#ifndef __DPL_DEBUGGER_INSTRUCTIONS_H
#define __DPL_DEBUGGER_INSTRUCTIONS_H

#include <raylib.h>

#include <dpl/program.h>
#include <dpl/value.h>

typedef struct
{
    Rectangle bounds;
    bool is_breakpoint;
    size_t ip;
    DPL_Instruction_Kind kind;
    const char* name;
    DPL_Value parameter0;
    DPL_Value parameter1;
    size_t parameter_count;
} DPLG_Instruction;

typedef struct
{
    DPLG_Instruction* items;
    size_t capacity;
    size_t count;

    DPL_MemoryValue_Pool pool;
} DPLG_Instructions;

void dplg_load_instructions(DPLG_Instructions *instructions, DPL_Program* program);

#endif // __DPL_DEBUGGER_INSTRUCTIONS_H