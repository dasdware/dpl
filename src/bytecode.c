#include "bytecode.h"
#include "error.h"

void dplb_init(DPL_ByteCode* bytecode) {
    bytecode->version = 1;
}

void dplb_free(DPL_ByteCode* bytecode) {
    nob_da_free(bytecode->constants);
    nob_da_free(bytecode->code);
}

size_t _dplb_add_number_constant(DPL_ByteCode *bytecode, double value) {
    size_t offset = bytecode->constants.count;
    nob_da_append_many(&bytecode->constants, &value, sizeof(value));
    return offset;
}

void dplb_write(DPL_ByteCode *bytecode, DPL_Instruction_Kind kind) {
    nob_da_append(&bytecode->code, (uint8_t) kind);
}

void dplb_write_noop(DPL_ByteCode *bytecode) {
    dplb_write(bytecode, INST_NOOP);
}

void dplb_write_push_number(DPL_ByteCode *bytecode, double value) {
    dplb_write(bytecode, INST_PUSH_NUMBER);

    size_t offset = _dplb_add_number_constant(bytecode, value);
    nob_da_append_many(&bytecode->code, &offset, sizeof(offset));
}

void dplb_write_negate(DPL_ByteCode *bytecode) {
    dplb_write(bytecode, INST_NEGATE);
}

void dplb_write_add(DPL_ByteCode *bytecode) {
    dplb_write(bytecode, INST_ADD);
}

void dplb_write_subtract(DPL_ByteCode *bytecode) {
    dplb_write(bytecode, INST_SUBTRACT);
}

void dplb_write_multiply(DPL_ByteCode *bytecode) {
    dplb_write(bytecode, INST_MULTIPLY);
}

void dplb_write_divide(DPL_ByteCode *bytecode) {
    dplb_write(bytecode, INST_DIVIDE);
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

    dpl_error("ERROR: Cannot get name for instruction kind %d.\n", kind);
}

void dplb_print(DPL_ByteCode *bytecode) {
    printf("%zu bytes in constant chunk:\n", bytecode->constants.count);
    for (size_t i = 0; i < bytecode->constants.count; ++i) {
        printf(" %02X", bytecode->constants.items[i]);
    }
    printf("\n\n");

    printf("%zu bytes in code chunk:\n", bytecode->code.count);
    for (size_t i = 0; i < bytecode->code.count; ++i) {
        printf(" %02X", bytecode->code.items[i]);

    }
    printf("\n\n");

    size_t ip = 0;
    while (ip < bytecode->code.count) {
        DPL_Instruction_Kind kind = bytecode->code.items[ip];
        ++ip;

        printf("%s", _dplb_inst_kind_name(kind));
        switch (kind) {
        case INST_PUSH_NUMBER: {
            size_t offset = *(bytecode->code.items + ip);
            ip += sizeof(offset);

            double value = *(double*)(bytecode->constants.items + offset);

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
}

void dplb_generate(DPL_CallTree* tree, DPL_CallTree_Node* node, DPL_ByteCode* bytecode) {
    switch (node->kind)
    {
    case CALLTREE_NODE_VALUE: {
        if (node->type_handle == tree->types->number_handle) {
            double value = atof(nob_temp_sv_to_cstr(node->as.value.ast_node->as.literal.value.text));
            dplb_write_push_number(bytecode, value);
        } else {
            DPL_Type* type = dplt_find_by_handle(tree->types, node->type_handle);
            dpl_error("Cannot generate bytecode for value node of type "SV_Fmt".", SV_Arg(type->name));
        }
    }
    break;
    case CALLTREE_NODE_FUNCTION: {
        DPL_Function* function = dplf_find_by_handle(tree->functions, node->as.function.function_handle);
        for (size_t i = 0; i < node->as.function.arguments.count; ++i) {
            dplb_generate(tree, node->as.function.arguments.items[i], bytecode);
        }

        if (nob_sv_eq(function->name, nob_sv_from_cstr("negate"))) {
            dplb_write_negate(bytecode);
        } else if (nob_sv_eq(function->name, nob_sv_from_cstr("add"))) {
            dplb_write_add(bytecode);
        } else if (nob_sv_eq(function->name, nob_sv_from_cstr("subtract"))) {
            dplb_write_subtract(bytecode);
        } else if (nob_sv_eq(function->name, nob_sv_from_cstr("multiply"))) {
            dplb_write_multiply(bytecode);
        } else if (nob_sv_eq(function->name, nob_sv_from_cstr("divide"))) {
            dplb_write_divide(bytecode);
        } else {
            dpl_error("Cannot generate bytecode for function "SV_Fmt".\n", SV_Arg(function->name));
        }
    }
    break;

    }
}
