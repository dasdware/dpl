#include "program.h"

void dplp_init(DPL_Program* program) {
    program->version = 1;
}

void dplp_free(DPL_Program* program) {
    nob_da_free(program->constants);
    nob_da_free(program->code);
}

size_t _dplp_add_number_constant(DPL_Program *program, double value) {
    size_t offset = program->constants.count;
    nob_da_append_many(&program->constants, &value, sizeof(value));
    return offset;
}

void dplp_write(DPL_Program *program, DPL_Instruction_Kind kind) {
    nob_da_append(&program->code, (uint8_t) kind);
}

void dplp_write_noop(DPL_Program *program) {
    dplp_write(program, INST_NOOP);
}

void dplp_write_push_number(DPL_Program *program, double value) {
    dplp_write(program, INST_PUSH_NUMBER);

    size_t offset = _dplp_add_number_constant(program, value);
    nob_da_append_many(&program->code, &offset, sizeof(offset));
}

void dplp_write_pop(DPL_Program* program) {
    dplp_write(program, INST_POP);
}

void dplp_write_negate(DPL_Program *program) {
    dplp_write(program, INST_NEGATE);
}

void dplp_write_add(DPL_Program *program) {
    dplp_write(program, INST_ADD);
}

void dplp_write_subtract(DPL_Program *program) {
    dplp_write(program, INST_SUBTRACT);
}

void dplp_write_multiply(DPL_Program *program) {
    dplp_write(program, INST_MULTIPLY);
}

void dplp_write_divide(DPL_Program *program) {
    dplp_write(program, INST_DIVIDE);
}

void dplp_write_call_external(DPL_Program *program, size_t external_num) {
    dplp_write(program, INST_CALL_EXTERNAL);
    nob_da_append(&program->code, (uint8_t) external_num);
}

const char* _dplp_inst_kind_name(DPL_Instruction_Kind kind) {
    switch (kind) {
    case INST_NOOP:
        return "INST_NOOP";
    case INST_PUSH_NUMBER:
        return "INST_PUSH_NUMBER";
    case INST_POP:
        return "INST_POP";
    case INST_NEGATE:
        return "INST_NEGATE";
    case INST_ADD:
        return "INST_ADD";
    case INST_SUBTRACT:
        return "INST_SUBTRACT";
    case INST_MULTIPLY:
        return "INST_MULTIPLY";
    case INST_DIVIDE:
        return "INST_DIVIDE";
    case INST_CALL_EXTERNAL:
        return "INST_CALL_EXTERNAL";
    }

    fprintf(stderr, "ERROR: Cannot get name for instruction kind %d.\n", kind);
    exit(1);
}

void dplp_print(DPL_Program *program) {
    printf("%zu bytes in constant chunk:\n", program->constants.count);
    for (size_t i = 0; i < program->constants.count; ++i) {
        printf(" %02X", program->constants.items[i]);
    }
    printf("\n\n");

    printf("%zu bytes in code chunk:\n", program->code.count);
    for (size_t i = 0; i < program->code.count; ++i) {
        printf(" %02X", program->code.items[i]);

    }
    printf("\n\n");

    size_t ip = 0;
    while (ip < program->code.count) {
        DPL_Instruction_Kind kind = program->code.items[ip];
        ++ip;

        printf("%s", _dplp_inst_kind_name(kind));
        switch (kind) {
        case INST_PUSH_NUMBER: {
            size_t offset = *(program->code.items + ip);
            ip += sizeof(offset);

            double value = *(double*)(program->constants.items + offset);

            printf(" %zu: %f ", offset, value);
        }
        break;
        case INST_NOOP:
        case INST_POP:
        case INST_NEGATE:
        case INST_ADD:
        case INST_SUBTRACT:
        case INST_MULTIPLY:
        case INST_DIVIDE:
            break;
        case INST_CALL_EXTERNAL: {
            uint8_t external_num = *(program->code.items + ip);
            ip += sizeof(external_num);
            printf(" %u", external_num);
        }
        break;
        }

        printf("\n");
    }
    printf("\n");
}

bool _dplp_save_chunk(FILE* out, const char* name, size_t size, void* data)
{
    assert(strlen(name) == 4);

    fwrite(name, sizeof(char), 4, out);
    fwrite(&size, sizeof(size_t), 1, out);
    fwrite(data, sizeof(uint8_t), size, out);

    return true;
}

bool dplp_save(DPL_Program* program, const char* file_name)
{
    FILE *out = fopen(file_name, "wb");

    _dplp_save_chunk(out, "HEAD", sizeof(program->version), &program->version);
    _dplp_save_chunk(out, "CONS", program->constants.count, program->constants.items);
    _dplp_save_chunk(out, "CODE", program->code.count, program->code.items);

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
            fprintf(stderr, "This version of dpl does not support program chunks of type \"%s\". Chunk will be ignored.\n", chunk.name);
        }
    }

    nob_da_free(chunk);
    fclose(in);

    (void)program;
    (void)file_name;
    return true;
}