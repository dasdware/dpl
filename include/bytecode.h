#ifndef __DPL_BYTECODE_H
#define __DPL_BYTECODE_H

#include "calltree.h"

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
} DPL_ByteCode;

void dplb_init(DPL_ByteCode *bytecode);
void dplb_free(DPL_ByteCode *bytecode);

void dplb_write(DPL_ByteCode *bytecode, DPL_Instruction_Kind kind);

void dplb_write_noop(DPL_ByteCode *bytecode);

void dplb_write_push_number(DPL_ByteCode *bytecode, double value);

void dplb_write_negate(DPL_ByteCode *bytecode);

void dplb_write_add(DPL_ByteCode *bytecode);
void dplb_write_subtract(DPL_ByteCode *bytecode);
void dplb_write_multiply(DPL_ByteCode *bytecode);
void dplb_write_divide(DPL_ByteCode *bytecode);

void dplb_generate(DPL_CallTree *tree, DPL_CallTree_Node *node, DPL_ByteCode *bytecode);

void dplb_print(DPL_ByteCode *bytecode);

#endif // __DPL_BYTECODE_H