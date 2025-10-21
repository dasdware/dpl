#ifndef __DPL_DEBUGGER_INSTRUCTIONS_H
#define __DPL_DEBUGGER_INSTRUCTIONS_H

#include <raylib.h>

#include <dpl/program.h>

typedef enum
{
    PARAMETER_EMPTY,
    PARAMETER_NUMBER,
    PARAMETER_BOOLEAN,
    PARAMETER_STRING,
    PARAMETER_SIZE,
    PARAMETER_CSTR,
} DPLG_Instruction_Parameter_Kind;

typedef struct
{
    DPLG_Instruction_Parameter_Kind kind;

    union
    {
        double number;
        bool boolean;
        Nob_String_View string;
        size_t size;
        const char* cstr;
    } as;
} DPLG_Instruction_Parameter;

typedef struct
{
    Rectangle bounds;
    bool is_breakpoint;
    size_t ip;
    DPL_Instruction_Kind kind;
    const char* name;
    DPLG_Instruction_Parameter parameter0;
    DPLG_Instruction_Parameter parameter1;
} DPLG_Instruction;

typedef struct
{
    DPLG_Instruction* items;
    size_t capacity;
    size_t count;
} DPLG_Instructions;

void dplg_load_instructions(DPLG_Instructions *instructions, DPL_Program* program);

#endif // __DPL_DEBUGGER_INSTRUCTIONS_H