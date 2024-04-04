#ifndef __DPL_PROGRAM_H
#define __DPL_PROGRAM_H

#include <stdint.h>

#include "nob.h"

typedef enum
{
    INST_NOOP,
    INST_PUSH_NUMBER,
    INST_NEGATE,
    INST_ADD,
    INST_SUBTRACT,
    INST_MULTIPLY,
    INST_DIVIDE,
} DPL_Instruction_Kind;

typedef struct
{
    uint8_t *items;
    size_t count;
    size_t capacity;
} DPL_Bytes;

typedef struct
{
    uint8_t version;
    DPL_Bytes code;
    DPL_Bytes constants;
} DPL_Program;

void dplp_init(DPL_Program *program);
void dplp_free(DPL_Program *program);

void dplp_write(DPL_Program *program, DPL_Instruction_Kind kind);

void dplp_write_noop(DPL_Program *program);

void dplp_write_push_number(DPL_Program *program, double value);

void dplp_write_negate(DPL_Program *program);

void dplp_write_add(DPL_Program *program);
void dplp_write_subtract(DPL_Program *program);
void dplp_write_multiply(DPL_Program *program);
void dplp_write_divide(DPL_Program *program);

void dplp_print(DPL_Program *program);

bool dplp_save(DPL_Program* program, const char* file_name);
bool dplp_load(DPL_Program* program, const char* file_name);

#endif // __DPL_PROGRAM_H