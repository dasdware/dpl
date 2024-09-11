#ifndef __DPL_PROGRAM_H
#define __DPL_PROGRAM_H

#include <stdint.h>

#include "nob.h"

#include "dw_byte_buffer.h"
#include "dw_array.h"
#include "value.h"

typedef enum
{
    INST_NOOP,
    INST_PUSH_NUMBER,
    INST_PUSH_STRING,
    INST_PUSH_BOOLEAN,
    INST_POP,
    INST_NEGATE,
    INST_ADD,
    INST_SUBTRACT,
    INST_MULTIPLY,
    INST_DIVIDE,
    INST_CALL_EXTERNAL,
    INST_CALL_USER,
    INST_PUSH_LOCAL,
    INST_STORE_LOCAL,
    INST_POP_SCOPE,
    INST_RETURN,
} DPL_Instruction_Kind;

typedef struct {
    DPL_ValueKind kind;
    size_t offset;
} DPL_Constant;

typedef da_array(DPL_Constant) DPL_Constants_Dictionary;

typedef struct
{
    uint8_t version;
    uint64_t entry;

    DW_ByteBuffer code;
    DW_ByteBuffer constants;

    DPL_Constants_Dictionary constants_dictionary;
} DPL_Program;

void dplp_init(DPL_Program *program);
void dplp_free(DPL_Program *program);

void dplp_write(DPL_Program *program, DPL_Instruction_Kind kind);

void dplp_write_noop(DPL_Program *program);

void dplp_write_push_number(DPL_Program *program, double value);
void dplp_write_push_string(DPL_Program *program, const char* value);
void dplp_write_push_boolean(DPL_Program *program, bool value);
void dplp_write_push_local(DPL_Program *program, size_t scope_index);
void dplp_write_pop(DPL_Program* program);
void dplp_write_pop_scope(DPL_Program* program, size_t n);

void dplp_write_negate(DPL_Program *program);

void dplp_write_add(DPL_Program *program);
void dplp_write_subtract(DPL_Program *program);
void dplp_write_multiply(DPL_Program *program);
void dplp_write_divide(DPL_Program *program);

void dplp_write_call_external(DPL_Program *program, size_t external_num);
void dplp_write_call_user(DPL_Program *program, size_t arity, size_t ip_begin);
void dplp_write_return(DPL_Program* program);

void dplp_write_store_local(DPL_Program *program, size_t scope_index);

const char* dplp_inst_kind_name(DPL_Instruction_Kind kind);

void dplp_print_escaped_string(const char* value, size_t length);
void dplp_print(DPL_Program *program);

bool dplp_save(DPL_Program* program, const char* file_name);
bool dplp_load(DPL_Program* program, const char* file_name);

#endif // __DPL_PROGRAM_H