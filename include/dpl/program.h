#ifndef __DPL_PROGRAM_H
#define __DPL_PROGRAM_H

#include <stdint.h>

#include <nobx.h>

#include "dw_byte_buffer.h"
#include "value.h"

#include <dpl/intrinsics.h>

typedef enum
{
    INST_NOOP,
    INST_PUSH_NUMBER,
    INST_PUSH_STRING,
    INST_PUSH_BOOLEAN,
    INST_POP,
    INST_NEGATE,
    INST_NOT,
    INST_ADD,
    INST_SUBTRACT,
    INST_MULTIPLY,
    INST_DIVIDE,
    INST_LESS,
    INST_LESS_EQUAL,
    INST_GREATER,
    INST_GREATER_EQUAL,
    INST_EQUAL,
    INST_NOT_EQUAL,
    INST_CALL_INTRINSIC,
    INST_CALL_USER,
    INST_PUSH_LOCAL,
    INST_STORE_LOCAL,
    INST_POP_SCOPE,
    INST_RETURN,
    INST_JUMP,
    INST_JUMP_IF_FALSE,
    INST_JUMP_IF_TRUE,
    INST_JUMP_LOOP,
    INST_CREATE_OBJECT,
    INST_LOAD_FIELD,
    INST_INTERPOLATION,
    INST_BEGIN_ARRAY,
    INST_END_ARRAY,
    INST_CONCAT_ARRAY,
    INST_SPREAD,
} DPL_Instruction_Kind;

typedef struct
{
    DPL_ValueKind kind;
    size_t offset;
} DPL_Constant;

typedef struct
{
    DPL_Constant *items;
    size_t count;
    size_t capacity;
} DPL_Constants_Dictionary;

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
void dplp_write_push_string(DPL_Program *program, const char *value);
void dplp_write_push_boolean(DPL_Program *program, bool value);
void dplp_write_push_local(DPL_Program *program, size_t scope_index);
void dplp_write_pop(DPL_Program *program);
void dplp_write_pop_scope(DPL_Program *program, size_t n);

void dplp_write_create_object(DPL_Program *program, size_t field_count);
void dplp_write_load_field(DPL_Program *program, size_t field_index);

void dplp_write_negate(DPL_Program *program);

void dplp_write_add(DPL_Program *program);
void dplp_write_subtract(DPL_Program *program);
void dplp_write_multiply(DPL_Program *program);
void dplp_write_divide(DPL_Program *program);

void dplp_write_call_intrinsic(DPL_Program *program, DPL_Intrinsic_Kind intrinsic);
void dplp_write_call_user(DPL_Program *program, size_t arity, size_t ip_begin);
void dplp_write_return(DPL_Program *program);

void dplp_write_store_local(DPL_Program *program, size_t scope_index);

size_t dplp_write_jump(DPL_Program *program, DPL_Instruction_Kind jump_kind);
void dplp_patch_jump(DPL_Program *program, size_t offset);
void dplp_write_loop(DPL_Program *program, size_t target);

void dplp_write_interpolation(DPL_Program *program, size_t count);

void dplp_write_begin_array(DPL_Program *program);
void dplp_write_end_array(DPL_Program *program);
void dplp_write_concat_array(DPL_Program *program);
void dplp_write_spread(DPL_Program *program);

const char *dplp_inst_kind_name(DPL_Instruction_Kind kind);

void dplp_print_escaped_string(const char *value, size_t length);
void dplp_print_stream_instruction(DW_ByteStream *code, DW_ByteStream *constants);
void dplp_print(DPL_Program *program);

bool dplp_save(DPL_Program *program, const char *file_name);
bool dplp_load(DPL_Program *program, const char *file_name);

#endif // __DPL_PROGRAM_H