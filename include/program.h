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

void dplb_init(DPL_Program *program);
void dplb_free(DPL_Program *program);

void dplb_write(DPL_Program *program, DPL_Instruction_Kind kind);

void dplb_write_noop(DPL_Program *program);

void dplb_write_push_number(DPL_Program *program, double value);

void dplb_write_negate(DPL_Program *program);

void dplb_write_add(DPL_Program *program);
void dplb_write_subtract(DPL_Program *program);
void dplb_write_multiply(DPL_Program *program);
void dplb_write_divide(DPL_Program *program);

void dplb_print(DPL_Program *program);

#endif // __DPL_PROGRAM_H