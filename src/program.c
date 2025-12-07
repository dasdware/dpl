#ifdef DPL_LEAKCHECK
#include <stb_leakcheck.h>
#endif

#include <dw_error.h>
#include <dpl/program.h>

void dplp_init(DPL_Program *program)
{
    program->version = 1;
}

void dplp_free(DPL_Program *program)
{
    nob_da_free(program->constants);
    nob_da_free(program->code);
    nob_da_free(program->constants_dictionary);
}

bool _dplp_find_number_constant(DPL_Program *program, double value, size_t *output)
{
    for (size_t i = 0; i < program->constants_dictionary.count; ++i)
    {
        DPL_Constant constant = program->constants_dictionary.items[i];
        if (constant.kind == VALUE_NUMBER && dpl_value_number_equals(bb_read_f64(program->constants, constant.offset), value))
        {
            *output = constant.offset;
            return true;
        }
    }

    return false;
}

bool _dplp_find_string_constant(DPL_Program *program, Nob_String_View value, size_t *output)
{
    for (size_t i = 0; i < program->constants_dictionary.count; ++i)
    {
        DPL_Constant constant = program->constants_dictionary.items[i];
        if (constant.kind == VALUE_STRING && dpl_value_string_equals(bb_read_sv(program->constants, constant.offset), value))
        {
            *output = constant.offset;
            return true;
        }
    }

    return false;
}

// size_t _dplp_add_number_constant(DPL_Program *program, double value) {
//     size_t offset = da_size(program->constants);
//     if (_dplp_find_number_constant(program, value, &offset)) {
//         return offset;
//     }

//     bb_write_f64(&program->constants, value);

//     DPL_Constant dictionary_entry = {
//         .kind = VALUE_NUMBER,
//         .offset = offset,
//     };
//     da_add(program->constants_dictionary, dictionary_entry);

//     return offset;
// }

size_t _dplp_add_string_constant(DPL_Program *program, const char *value)
{
    Nob_String_View value_view = nob_sv_from_cstr(value);
    size_t offset = program->constants.count;
    if (_dplp_find_string_constant(program, value_view, &offset))
    {
        return offset;
    }

    bb_write_sv(&program->constants, value_view);

    DPL_Constant dictionary_entry = {
        .offset = offset,
        .kind = VALUE_STRING,
    };
    nob_da_append(&program->constants_dictionary, dictionary_entry);

    return offset;
}

void dplp_write(DPL_Program *program, DPL_Instruction_Kind kind)
{
    bb_write_u8(&program->code, kind);
}

void dplp_write_noop(DPL_Program *program)
{
    bb_write_u8(&program->code, INST_NOOP);
}

void dplp_write_push_number(DPL_Program *program, double value)
{
    bb_write_u8(&program->code, INST_PUSH_NUMBER);
    bb_write_f64(&program->code, value);
    // bb_write_u64(&program->code, _dplp_add_number_constant(program, value));
}

void dplp_write_push_string(DPL_Program *program, const char *value)
{
    bb_write_u8(&program->code, INST_PUSH_STRING);
    bb_write_u64(&program->code, _dplp_add_string_constant(program, value));
}

void dplp_write_push_boolean(DPL_Program *program, bool value)
{
    bb_write_u8(&program->code, INST_PUSH_BOOLEAN);
    bb_write_u8(&program->code, value ? 1 : 0);
}

void dplp_write_create_object(DPL_Program *program, size_t field_count)
{
    bb_write_u8(&program->code, INST_CREATE_OBJECT);
    bb_write_u8(&program->code, field_count);
}

void dplp_write_load_field(DPL_Program *program, size_t field_index)
{
    bb_write_u8(&program->code, INST_LOAD_FIELD);
    bb_write_u8(&program->code, field_index);
}

void dplp_write_push_local(DPL_Program *program, size_t scope_index)
{
    bb_write_u8(&program->code, INST_PUSH_LOCAL);
    bb_write_u64(&program->code, scope_index);
}

void dplp_write_pop(DPL_Program *program)
{
    bb_write_u8(&program->code, INST_POP);
}

void dplp_write_pop_scope(DPL_Program *program, size_t n)
{
    bb_write_u8(&program->code, INST_POP_SCOPE);
    bb_write_u64(&program->code, n);
}

void dplp_write_negate(DPL_Program *program)
{
    bb_write_u8(&program->code, INST_NEGATE);
}

void dplp_write_add(DPL_Program *program)
{
    bb_write_u8(&program->code, INST_ADD);
}

void dplp_write_subtract(DPL_Program *program)
{
    bb_write_u8(&program->code, INST_SUBTRACT);
}

void dplp_write_multiply(DPL_Program *program)
{
    bb_write_u8(&program->code, INST_MULTIPLY);
}

void dplp_write_divide(DPL_Program *program)
{
    bb_write_u8(&program->code, INST_DIVIDE);
}

void dplp_write_call_intrinsic(DPL_Program *program, DPL_Intrinsic_Kind intrinsic)
{
    bb_write_u8(&program->code, INST_CALL_INTRINSIC);
    bb_write_u8(&program->code, intrinsic);
}

void dplp_write_call_user(DPL_Program *program, size_t arity, size_t ip_begin)
{
    bb_write_u8(&program->code, INST_CALL_USER);
    bb_write_u8(&program->code, arity);
    bb_write_u64(&program->code, ip_begin);
}

void dplp_write_return(DPL_Program *program)
{
    bb_write_u8(&program->code, INST_RETURN);
}

void dplp_write_store_local(DPL_Program *program, size_t scope_index)
{
    bb_write_u8(&program->code, INST_STORE_LOCAL);
    bb_write_u64(&program->code, scope_index);
}

size_t dplp_write_jump(DPL_Program *program, DPL_Instruction_Kind jump_kind)
{
    dplp_write(program, jump_kind);
    bb_write_u16(&program->code, 0);
    return program->code.count - 2;
}

void dplp_patch_jump(DPL_Program *program, size_t offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = program->code.count - offset - 2;

    if (jump > UINT16_MAX)
    {
        DW_ERROR("Cannot generate jumps larger then %u bytes.", UINT16_MAX);
    }

    uint16_t u16_jump = jump;
    *((uint16_t *)(program->code.items + offset)) = u16_jump;
}

void dplp_write_loop(DPL_Program *program, size_t target)
{
    dplp_write(program, INST_JUMP_LOOP);

    int jump = program->code.count - target + 2;
    if (jump > UINT16_MAX)
    {
        DW_ERROR("Cannot generate jumps larger then %u bytes.", UINT16_MAX);
    }

    bb_write_u16(&program->code, jump);
}

void dplp_write_interpolation(DPL_Program *program, size_t count)
{
    dplp_write(program, INST_INTERPOLATION);
    bb_write_u8(&program->code, count);
}

void dplp_write_begin_array(DPL_Program *program)
{
    dplp_write(program, INST_BEGIN_ARRAY);
}

void dplp_write_end_array(DPL_Program *program)
{
    dplp_write(program, INST_END_ARRAY);
}

void dplp_write_concat_array(DPL_Program *program)
{
    dplp_write(program, INST_CONCAT_ARRAY);
}

void dplp_write_spread(DPL_Program *program)
{
    dplp_write(program, INST_SPREAD);
}

const char *dplp_inst_kind_name(DPL_Instruction_Kind kind)
{
    switch (kind)
    {
    case INST_NOOP:
        return "NOOP";
    case INST_PUSH_NUMBER:
        return "PUSH_NUMBER";
    case INST_PUSH_STRING:
        return "PUSH_STRING";
    case INST_PUSH_BOOLEAN:
        return "PUSH_BOOLEAN";
    case INST_POP:
        return "POP";
    case INST_NEGATE:
        return "NEGATE";
    case INST_NOT:
        return "NOT";
    case INST_ADD:
        return "ADD";
    case INST_SUBTRACT:
        return "SUBTRACT";
    case INST_MULTIPLY:
        return "MULTIPLY";
    case INST_DIVIDE:
        return "DIVIDE";
    case INST_LESS:
        return "LESS";
    case INST_LESS_EQUAL:
        return "LESS_EQUAL";
    case INST_GREATER:
        return "GREATER";
    case INST_GREATER_EQUAL:
        return "GREATER_EQUAL";
    case INST_EQUAL:
        return "EQUAL";
    case INST_NOT_EQUAL:
        return "NOT_EQUAL";
    case INST_CALL_INTRINSIC:
        return "CALL_INTRINSIC";
    case INST_CALL_USER:
        return "CALL_USER";
    case INST_RETURN:
        return "RETURN";
    case INST_PUSH_LOCAL:
        return "PUSH_LOCAL";
    case INST_STORE_LOCAL:
        return "STORE_LOCAL";
    case INST_POP_SCOPE:
        return "POP_SCOPE";
    case INST_JUMP:
        return "JUMP";
    case INST_JUMP_IF_FALSE:
        return "JUMP_IF_FALSE";
    case INST_JUMP_IF_TRUE:
        return "JUMP_IF_TRUE";
    case INST_JUMP_LOOP:
        return "JUMP_LOOP";
    case INST_CREATE_OBJECT:
        return "CREATE_OBJECT";
    case INST_LOAD_FIELD:
        return "LOAD_FIELD";
    case INST_INTERPOLATION:
        return "INTERPOLATION";
    case INST_BEGIN_ARRAY:
        return "BEGIN_ARRAY";
    case INST_END_ARRAY:
        return "END_ARRAY";
    case INST_CONCAT_ARRAY:
        return "CONCAT_ARRAY";
    case INST_SPREAD:
        return "SPREAD";
    default:
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
}

void dplp_print_escaped_string(const char *value, size_t length)
{
    char *pos = (char *)value;
    for (size_t i = 0; i < length; ++i)
    {
        switch (*pos)
        {
        case '\n':
            printf("\\n");
            break;
        case '\r':
            printf("\\r");
            break;
        case '\t':
            printf("\\t");
            break;
        default:
            printf("%c", *pos);
        }
        ++pos;
    }
}

void _dplp_print_constant(DPL_Program *program, size_t i)
{
    printf(" #%zu: ", i);
    DPL_Constant constant = program->constants_dictionary.items[i];
    switch (constant.kind)
    {
    case VALUE_STRING:
        dpl_value_print_string(bb_read_sv(program->constants, constant.offset));
        break;
    case VALUE_NUMBER:
    case VALUE_BOOLEAN:
    case VALUE_OBJECT:
    case VALUE_ARRAY:
        // numbers, booleans or objects will never occur in constant dictionary
        break;
    }
    printf(" (offset: %zu)\n", constant.offset);
}

void dplp_print_stream_instruction(DW_ByteStream *code, DW_ByteStream *constants)
{
    printf("[%04zu] ", code->position);
    DPL_Instruction_Kind kind = bs_read_u8(code);
    printf("%s", dplp_inst_kind_name(kind));

    switch (kind)
    {
    case INST_PUSH_NUMBER:
    {
        double value = bs_read_f64(code);
        printf(" %f ", value);
    }
    break;
    case INST_PUSH_STRING:
    {
        constants->position = bs_read_u64(code);
        printf(" %zu: ", constants->position);
        size_t length = bs_read_u64(constants);
        printf("(length: %zu, value: \"", length);
        dplp_print_escaped_string(
            (char *)(constants->buffer.items + constants->position),
            length);
        printf("\")");
    }
    break;
    case INST_PUSH_BOOLEAN:
    {
        uint8_t value = bs_read_u8(code);
        printf(" %s", (value == 1) ? "true" : "false");
    }
    break;
    case INST_PUSH_LOCAL:
    {
        size_t scope_index = bs_read_u64(code);
        printf(" %zu", scope_index);
    }
    break;
    case INST_CREATE_OBJECT:
    {
        size_t field_count = bs_read_u8(code);
        printf(" %zu", field_count);
    }
    break;
    case INST_LOAD_FIELD:
    {
        size_t field_index = bs_read_u8(code);
        printf(" %zu", field_index);
    }
    break;
    case INST_NOOP:
    case INST_POP:
    case INST_NEGATE:
    case INST_NOT:
    case INST_ADD:
    case INST_SUBTRACT:
    case INST_MULTIPLY:
    case INST_DIVIDE:
    case INST_LESS:
    case INST_LESS_EQUAL:
    case INST_GREATER:
    case INST_GREATER_EQUAL:
    case INST_EQUAL:
    case INST_NOT_EQUAL:
    case INST_RETURN:
    case INST_BEGIN_ARRAY:
    case INST_END_ARRAY:
    case INST_CONCAT_ARRAY:
    case INST_SPREAD:
        break;
    case INST_CALL_INTRINSIC:
    {
        printf(" %s", dpl_intrinsic_kind_name(bs_read_u8(code)));
    }
    break;
    case INST_CALL_USER:
    {
        uint8_t arity = bs_read_u8(code);
        size_t begin_ip = bs_read_u64(code);
        printf(" %u %zu", arity, begin_ip);
    }
    break;
    case INST_STORE_LOCAL:
    {
        size_t scope_index = bs_read_u64(code);
        printf(" %zu", scope_index);
    }
    break;
    case INST_POP_SCOPE:
    {
        size_t n = bs_read_u64(code);
        printf(" %zu", n);
    }
    break;
    case INST_JUMP:
    case INST_JUMP_IF_FALSE:
    case INST_JUMP_IF_TRUE:
    case INST_JUMP_LOOP:
    {
        uint16_t offset = bs_read_u16(code);
        printf(" %u", offset);
    }
    break;
    case INST_INTERPOLATION:
    {
        uint8_t count = bs_read_u8(code);
        printf(" %u", count);
    }
    break;
    default:
        DW_UNIMPLEMENTED_MSG("%s", dplp_inst_kind_name(kind));
    }

    printf("\n");
}

void dplp_print(DPL_Program *program)
{
    printf("============ PROGRAM ============\n");
    printf("        Version: %u\n", program->version);
    printf("          Entry: %zu\n", program->entry);

    printf("----- CONSTANTS DICTIONARY ------\n");
    printf("           Size: %zu\n", program->constants_dictionary.count);
    printf("     Chunk size: %zu\n", program->constants.count);
    for (size_t i = 0; i < program->constants_dictionary.count; ++i)
    {
        _dplp_print_constant(program, i);
    }

    printf("------------- CODE --------------\n");
    printf("     Chunk size: %zu\n", program->code.count);

    DW_ByteStream constants = {
        .buffer = program->constants,
        .position = 0,
    };
    DW_ByteStream code = {
        .buffer = program->code,
        .position = 0,
    };
    while (!bs_at_end(&code))
    {
        dplp_print_stream_instruction(&code, &constants);
        continue;
    }
    printf("=================================\n");
    printf("\n");
}

bool _dplp_save_chunk(FILE *out, const char *name, DW_ByteBuffer buffer)
{
    assert(strlen(name) == 4);

    fwrite(name, sizeof(char), 4, out);
    bb_save(out, buffer);

    return true;
}

bool dplp_save(DPL_Program *program, const char *file_name)
{
    FILE *out = fopen(file_name, "wb");

    DW_ByteBuffer header = {0};
    bb_write_u8(&header, program->version);
    bb_write_u64(&header, program->entry);
    _dplp_save_chunk(out, "HEAD", header);
    nob_da_free(header);

    _dplp_save_chunk(out, "CONS", program->constants);
    _dplp_save_chunk(out, "CODE", program->code);

    fclose(out);

    return true;
}

typedef struct
{
    char name[5];
    DW_ByteBuffer data;
} DPL_Loaded_Chunk;

bool _dplp_load_chunk(FILE *in, DPL_Loaded_Chunk *chunk)
{
    if (feof(in))
    {
        return false;
    }

    if (fread(chunk->name, 1, 4, in) < 4)
    {
        return false;
    }

    size_t count;
    if (fread(&count, sizeof(count), 1, in) < 1)
    {
        return false;
    }

    nob_da_reserve(&chunk->data, count);
    if (fread(chunk->data.items, 1, count, in) < count)
    {
        return false;
    }
    chunk->data.count = count;

    return true;
}

bool dplp_load(DPL_Program *program, const char *file_name)
{
    FILE *in = fopen(file_name, "rb");

    DPL_Loaded_Chunk chunk = {0};
    while (_dplp_load_chunk(in, &chunk))
    {
        if (strcmp(chunk.name, "HEAD") == 0)
        {
            program->version = chunk.data.items[0];
            program->entry = *(uint64_t *)(chunk.data.items + sizeof(program->version));
        }
        else if (strcmp(chunk.name, "CONS") == 0)
        {
            nob_da_append_many(&program->constants, chunk.data.items, chunk.data.count);
        }
        else if (strcmp(chunk.name, "CODE") == 0)
        {
            nob_da_append_many(&program->code, chunk.data.items, chunk.data.count);
        }
        else
        {
            DW_ERROR_MSGLN("This version of dpl does not support program chunks of type \"%s\". Chunk will be ignored.", chunk.name);
        }
    }

    nob_da_free(chunk.data);
    fclose(in);
    return true;
}