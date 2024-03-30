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

const char* _dplp_inst_kind_name(DPL_Instruction_Kind kind) {
    switch (kind) {
    case INST_NOOP:
        return "INST_NOOP";
    case INST_PUSH_NUMBER:
        return "INST_PUSH_NUMBER";
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
        case INST_NEGATE:
        case INST_ADD:
        case INST_SUBTRACT:
        case INST_MULTIPLY:
        case INST_DIVIDE:
            break;
        }

        printf("\n");
    }
    printf("\n");
}

// Credit: https://stackoverflow.com/a/51076933
/* this function will create a new name, replacing the existing extension
   by the given one.
   returned value should be `free()` after usage

   /!\ warning:
        * validity of parameters is not tested
        * return of strdup and malloc are not tested.
   */
char *_dplp_replace_ext(const char *org, const char *new_ext)
{
    char *ext;

    /* copy the original file */
    char *tmp = strdup(org);

    /* find last period in name */
    ext = strrchr(tmp, '.');

    /* if found, replace period with '\0', thus, we have a shorter string */
    if (ext) {
        *ext = '\0';
    }

    /* compute the new name size: size of name w/o ext + size of ext + 1
       for the final '\0' */
    size_t new_size = strlen(tmp) + strlen(new_ext) + 1;

    /* allocate memory for new name*/
    char *new_name = malloc(new_size);

    /* concatenate the two string */
    sprintf(new_name, "%s%s", tmp, new_ext);

    /* free tmp memory */
    free(tmp);

    /* return the new name */
    return new_name;
}

char* dplp_filename(const char* source_filename)
{
    return _dplp_replace_ext(source_filename, ".dplp");
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