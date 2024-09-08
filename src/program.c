#include "program.h"
#include "error.h"

void dplp_init(DPL_Program* program) {
    program->version = 1;
}

void dplp_free(DPL_Program* program) {
    nob_da_free(program->constants);
    nob_da_free(program->code);
}

bool _dplp_find_number_constant(DPL_Program* program, double value, size_t* output) {
    for (size_t i = 0; i < da_size(program->constants_dictionary); ++i) {
        DPL_Constant constant = program->constants_dictionary[i];
        if (constant.kind == VALUE_NUMBER
                && dpl_value_number_equals(bb_read_f64(program->constants, constant.offset), value)) {
            *output = constant.offset;
            return true;
        }
    }

    return false;
}

bool _dplp_find_string_constant(DPL_Program* program, Nob_String_View value, size_t* output) {
    for (size_t i = 0; i < da_size(program->constants_dictionary); ++i) {
        DPL_Constant constant = program->constants_dictionary[i];
        if (constant.kind == VALUE_STRING
                && dpl_value_string_equals(bb_read_sv(program->constants, constant.offset), value)) {
            *output = constant.offset;
            return true;
        }
    }

    return false;
}

size_t _dplp_add_number_constant(DPL_Program *program, double value) {
    size_t offset = program->constants.count;
    if (_dplp_find_number_constant(program, value, &offset)) {
        return offset;
    }

    bb_write_f64(&program->constants, value);

    DPL_Constant dictionary_entry = {
        .kind = VALUE_NUMBER,
        .offset = offset,
    };
    da_add(program->constants_dictionary, dictionary_entry);

    return offset;
}

size_t _dplp_add_string_constant(DPL_Program *program, const char* value) {
    Nob_String_View value_view = nob_sv_from_cstr(value);
    size_t offset = program->constants.count;
    if (_dplp_find_string_constant(program, value_view, &offset)) {
        return offset;
    }

    bb_write_sv(&program->constants, value_view);

    DPL_Constant dictionary_entry = {
        .offset = offset,
        .kind = VALUE_STRING,
    };
    da_add(program->constants_dictionary, dictionary_entry);

    return offset;
}

void dplp_write(DPL_Program *program, DPL_Instruction_Kind kind) {
    bb_write_u8(&program->code, kind);
}

void dplp_write_noop(DPL_Program *program) {
    bb_write_u8(&program->code, INST_NOOP);
}

void dplp_write_push_number(DPL_Program *program, double value) {
    bb_write_u8(&program->code, INST_PUSH_NUMBER);
    bb_write_u64(&program->code, _dplp_add_number_constant(program, value));
}

void dplp_write_push_string(DPL_Program *program, const char* value) {
    bb_write_u8(&program->code, INST_PUSH_STRING);
    bb_write_u64(&program->code, _dplp_add_string_constant(program, value));
}

void dplp_write_push_local(DPL_Program *program, size_t scope_index) {
    bb_write_u8(&program->code, INST_PUSH_LOCAL);
    bb_write_u64(&program->code, scope_index);
}

void dplp_write_pop(DPL_Program* program) {
    bb_write_u8(&program->code, INST_POP);
}

void dplp_write_pop_scope(DPL_Program* program, size_t n) {
    bb_write_u8(&program->code, INST_POP_SCOPE);
    bb_write_u64(&program->code, n);
}

void dplp_write_negate(DPL_Program *program) {
    bb_write_u8(&program->code, INST_NEGATE);
}

void dplp_write_add(DPL_Program *program) {
    bb_write_u8(&program->code, INST_ADD);
}

void dplp_write_subtract(DPL_Program *program) {
    bb_write_u8(&program->code, INST_SUBTRACT);
}

void dplp_write_multiply(DPL_Program *program) {
    bb_write_u8(&program->code, INST_MULTIPLY);
}

void dplp_write_divide(DPL_Program *program) {
    bb_write_u8(&program->code, INST_DIVIDE);
}

void dplp_write_call_external(DPL_Program *program, size_t external_num) {
    bb_write_u8(&program->code, INST_CALL_EXTERNAL);
    bb_write_u8(&program->code, external_num);
}

void dplp_write_call_user(DPL_Program *program, size_t arity, size_t ip_begin) {
    bb_write_u8(&program->code, INST_CALL_USER);
    bb_write_u8(&program->code, arity);
    bb_write_u64(&program->code, ip_begin);
}

void dplp_write_return(DPL_Program* program) {
    bb_write_u8(&program->code, INST_RETURN);
}

void dplp_write_store_local(DPL_Program *program, size_t scope_index) {
    bb_write_u8(&program->code, INST_STORE_LOCAL);
    bb_write_u64(&program->code, scope_index);
}

const char* dplp_inst_kind_name(DPL_Instruction_Kind kind) {
    switch (kind) {
    case INST_NOOP:
        return "NOOP";
    case INST_PUSH_NUMBER:
        return "PUSH_NUMBER";
    case INST_PUSH_STRING:
        return "PUSH_STRING";
    case INST_POP:
        return "POP";
    case INST_NEGATE:
        return "NEGATE";
    case INST_ADD:
        return "ADD";
    case INST_SUBTRACT:
        return "SUBTRACT";
    case INST_MULTIPLY:
        return "MULTIPLY";
    case INST_DIVIDE:
        return "DIVIDE";
    case INST_CALL_EXTERNAL:
        return "CALL_EXTERNAL";
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
    default:
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
}

void dplp_print_escaped_string(const char* value, size_t length) {
    char* pos = (char*)value;
    for (size_t i = 0; i < length; ++i) {
        switch (*pos) {
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

void _dplp_print_constant(DPL_Program* program, size_t i) {
    printf(" #%zu: ", i);
    DPL_Constant constant = program->constants_dictionary[i];
    switch (constant.kind) {
    case VALUE_NUMBER:
        dpl_value_print_number(bb_read_f64(program->constants, constant.offset));
        break;
    case VALUE_STRING:
        dpl_value_print_string(bb_read_sv(program->constants, constant.offset));
        break;
    }
    printf(" (offset: %zu)\n", constant.offset);
}

void dplp_print(DPL_Program *program) {
    printf("============ PROGRAM ============\n");
    printf("        Version: %u\n", program->version);
    printf("          Entry: %zu\n", program->entry);

    printf("----- CONSTANTS DICTIONARY ------\n");
    printf("           Size: %zu\n", da_size(program->constants_dictionary));
    printf("     Chunk size: %zu\n", program->constants.count);
    for (size_t i = 0; i < da_size(program->constants_dictionary); ++i) {
        _dplp_print_constant(program, i);
    }

    printf("------------- CODE --------------\n");
    printf("     Chunk size: %zu\n", program->code.count);

    size_t ip = 0;
    while (ip < program->code.count) {
        DPL_Instruction_Kind kind = program->code.items[ip];
        ++ip;

        printf("[%04zu] %s", ip - 1, dplp_inst_kind_name(kind));
        switch (kind) {
        case INST_PUSH_NUMBER: {
            size_t offset = *(program->code.items + ip);
            ip += sizeof(offset);

            double value = *(double*)(program->constants.items + offset);

            printf(" %zu: %f ", offset, value);
        }
        break;
        case INST_PUSH_STRING: {
            size_t offset = *(program->code.items + ip);
            size_t length = *(size_t*)(program->constants.items + offset);

            printf(" %zu: (length: %zu, value: \"", offset, length);
            dplp_print_escaped_string(
                (char*)(program->constants.items + offset + sizeof(length)),
                length);
            printf("\")");

            ip += sizeof(offset);
        }
        break;
        case INST_PUSH_LOCAL: {
            size_t scope_index = *(program->code.items + ip);

            printf(" %zu", scope_index);
            ip += sizeof(scope_index);
        }
        break;
        case INST_NOOP:
        case INST_POP:
        case INST_NEGATE:
        case INST_ADD:
        case INST_SUBTRACT:
        case INST_MULTIPLY:
        case INST_DIVIDE:
        case INST_RETURN:
            break;
        case INST_CALL_EXTERNAL: {
            uint8_t external_num = *(program->code.items + ip);
            ip += sizeof(external_num);
            printf(" %u", external_num);
        }
        break;
        case INST_CALL_USER: {
            uint8_t arity = *(program->code.items + ip);
            ip += sizeof(arity);
            size_t begin_ip = *(program->code.items + ip);
            ip += sizeof(begin_ip);

            printf(" %u %zu", arity, begin_ip);
        }
        break;
        case INST_STORE_LOCAL: {
            size_t scope_index = *(program->code.items + ip);

            printf(" %zu", scope_index);
            ip += sizeof(scope_index);
        }
        break;
        case INST_POP_SCOPE: {
            size_t n = *(program->code.items + ip);

            printf(" %zu", n);
            ip += sizeof(n);
        }
        break;
        default:
            DW_UNIMPLEMENTED_MSG("%s", dplp_inst_kind_name(kind));
        }

        printf("\n");
    }
    printf("=================================\n");
    printf("\n");
}

bool _dplp_save_chunk(FILE* out, const char* name, DW_ByteBuffer *buffer)
{
    assert(strlen(name) == 4);

    fwrite(name, sizeof(char), 4, out);
    bb_save(out, buffer);

    return true;
}

bool dplp_save(DPL_Program* program, const char* file_name)
{
    FILE *out = fopen(file_name, "wb");

    DW_ByteBuffer header = {0};
    bb_write_u8(&header, program->version);
    bb_write_u64(&header, program->entry);
    _dplp_save_chunk(out, "HEAD", &header);

    _dplp_save_chunk(out, "CONS", &program->constants);
    _dplp_save_chunk(out, "CODE", &program->code);

    fclose(out);

    return true;
}

typedef struct
{
    char name[5];

    size_t count;
    size_t capacity;
    uint8_t *items;
} DPL_Loaded_Chunk;

bool _dplp_load_chunk(FILE* in, DPL_Loaded_Chunk* chunk)
{
    if (feof(in)) {
        return false;
    }

    if (fread(chunk->name, 1, 4, in) < 4)
    {
        return false;
    }

    if (fread(&chunk->count, sizeof(chunk->count), 1, in) < 1)
    {
        return false;
    }

    bool need_realloc = false;
    while (chunk->capacity < chunk->count)
    {
        need_realloc = true;
        if (chunk->capacity == 0)
        {
            chunk->capacity = NOB_DA_INIT_CAP;
        }
        else
        {
            chunk->capacity *= 2;
        }
    }
    if (need_realloc)
    {
        chunk->items = NOB_REALLOC(chunk->items, chunk->capacity*sizeof(uint8_t));
    }

    if (fread(chunk->items, 1, chunk->count, in) < chunk->count)
    {
        return false;
    }

    return true;
}

bool dplp_load(DPL_Program* program, const char* file_name)
{
    FILE *in = fopen(file_name, "rb");

    DPL_Loaded_Chunk chunk = {0};
    while (_dplp_load_chunk(in, &chunk)) {
        if (strcmp(chunk.name, "HEAD") == 0)
        {
            program->version = chunk.items[0];
            program->entry = *(uint64_t*)(chunk.items + sizeof(program->version));
        }
        else if (strcmp(chunk.name, "CONS") == 0)
        {
            nob_da_append_many(&program->constants, chunk.items, chunk.count);
        }
        else if (strcmp(chunk.name, "CODE") == 0)
        {
            nob_da_append_many(&program->code, chunk.items, chunk.count);
        }
        else
        {
            DW_ERROR_MSGLN("This version of dpl does not support program chunks of type \"%s\". Chunk will be ignored.", chunk.name);
        }
    }

    nob_da_free(chunk);
    fclose(in);
    return true;
}