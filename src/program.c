#include "program.h"

void dplb_init(DPL_Program* program) {
    program->version = 1;
}

void dplb_free(DPL_Program* program) {
    nob_da_free(program->constants);
    nob_da_free(program->code);
}

size_t _dplb_add_number_constant(DPL_Program *program, double value) {
    size_t offset = program->constants.count;
    nob_da_append_many(&program->constants, &value, sizeof(value));
    return offset;
}

void dplb_write(DPL_Program *program, DPL_Instruction_Kind kind) {
    nob_da_append(&program->code, (uint8_t) kind);
}

void dplb_write_noop(DPL_Program *program) {
    dplb_write(program, INST_NOOP);
}

void dplb_write_push_number(DPL_Program *program, double value) {
    dplb_write(program, INST_PUSH_NUMBER);

    size_t offset = _dplb_add_number_constant(program, value);
    nob_da_append_many(&program->code, &offset, sizeof(offset));
}

void dplb_write_negate(DPL_Program *program) {
    dplb_write(program, INST_NEGATE);
}

void dplb_write_add(DPL_Program *program) {
    dplb_write(program, INST_ADD);
}

void dplb_write_subtract(DPL_Program *program) {
    dplb_write(program, INST_SUBTRACT);
}

void dplb_write_multiply(DPL_Program *program) {
    dplb_write(program, INST_MULTIPLY);
}

void dplb_write_divide(DPL_Program *program) {
    dplb_write(program, INST_DIVIDE);
}

const char* _dplb_inst_kind_name(DPL_Instruction_Kind kind) {
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

void dplb_print(DPL_Program *program) {
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

        printf("%s", _dplb_inst_kind_name(kind));
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
