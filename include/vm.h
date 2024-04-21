#ifndef __DPL_VM_H
#define __DPL_VM_H

#include "arena.h"
#include "program.h"
#include "string_table.h"

typedef enum {
    VALUE_NUMBER,
    VALUE_STRING,
} DPL_ValueKind;

typedef union {
    double number;
    DW_StringTable_Handle string;
} DPL_Value_As;

typedef struct {
    DPL_ValueKind kind;
    DPL_Value_As as;
} DPL_Value;

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

const char* dplv_value_kind_name(DPL_ValueKind kind);

DPL_Value dplv_number(double value);
const char* dplv_format_number(DPL_Value value);

DPL_Value dplv_string(DW_StringTable_Handle value);

void dplv_print_value(DPL_VirtualMachine* vm, DPL_Value value);

void dplv_init(DPL_VirtualMachine *vm, DPL_Program *program, struct DPL_ExternalFunctions *externals);
void dplv_free(DPL_VirtualMachine *vm);

void dplv_run(DPL_VirtualMachine *vm);

DPL_Value dplv_peek(DPL_VirtualMachine *vm);
void dplv_replace_top(DPL_VirtualMachine *vm, DPL_Value value);

#endif // __DPL_VM_H