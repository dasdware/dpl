#include "vm.h"
#include "externals.h"

const char* dplv_value_kind_name(DPL_ValueKind kind) {
    switch (kind) {
    case VALUE_NUMBER:
        return "number";
    case VALUE_STRING:
        return "string";
    }

    fprintf(stderr, "ERROR: Invalid value kind `%02X`.", kind);
    exit(1);
}

void dplv_print_value(DPL_VirtualMachine* vm, DPL_Value value) {
    (void) vm;
    const char* kind_name = dplv_value_kind_name(value.kind);
    switch (value.kind) {
    case VALUE_NUMBER:
        printf("[%s: %f]", kind_name, value.as.number);
        break;
    case VALUE_STRING: {
        printf("[%s: \"", kind_name);

        const char* pos = st_get(&vm->strings, value.as.string);
        size_t length = st_length(&vm->strings, value.as.string);

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
        printf("\"]");
    }
    break;
    default:
        fprintf(stderr, "Cannot debug print value of kind `%s`.\n", kind_name);
        exit(1);
    }
}

void dplv_init(DPL_VirtualMachine *vm, DPL_Program *program, struct DPL_ExternalFunctions *externals)
{
    vm->program = program;
    vm->externals = externals;

    if (vm->stack_capacity == 0)
    {
        vm->stack_capacity = 1024;
    }

    vm->stack = arena_alloc(&vm->memory, vm->stack_capacity * sizeof(DPL_Value));

    st_init(&vm->strings);
}

void dplv_free(DPL_VirtualMachine *vm)
{
    st_free(&vm->strings);
    arena_free(&vm->memory);
}

void dplv_run(DPL_VirtualMachine *vm)
{
#define TOP0 (vm->stack[vm->stack_top - 1])
#define TOP1 (vm->stack[vm->stack_top - 2])

    size_t ip = 0;
    while (ip < vm->program->code.count)
    {
        size_t ip_begin = ip;
        DPL_Instruction_Kind instruction = vm->program->code.items[ip];
        ++ip;
        switch (instruction)
        {
        case INST_NOOP:
            break;
        case INST_PUSH_NUMBER: {
            if (vm->stack_top >= vm->stack_capacity)
            {
                fprintf(stderr, "Fatal Error: Stack overflow in program execution.\n");
                exit(1);
            }

            size_t offset = *(vm->program->code.items + ip);
            ip += sizeof(offset);

            double value = *(double*)(vm->program->constants.items + offset);
            vm->stack[vm->stack_top].kind = VALUE_NUMBER;
            vm->stack[vm->stack_top].as.number = value;
            ++vm->stack_top;
        }
        break;
        case INST_PUSH_STRING: {
            size_t offset = *(vm->program->code.items + ip);
            ip += sizeof(offset);

            size_t length = *(size_t*)(vm->program->constants.items + offset);
            char* data = (char*)(vm->program->constants.items + offset + sizeof(length));

            vm->stack[vm->stack_top].kind = VALUE_STRING;
            vm->stack[vm->stack_top].as.string = st_allocate_lstr(&vm->strings, data, length);
            ++vm->stack_top;
        }
        break;
        case INST_NEGATE:
            TOP0.as.number = -TOP0.as.number;
            break;
        case INST_ADD:
            TOP1.as.number = TOP1.as.number + TOP0.as.number;
            --vm->stack_top;
            break;
        case INST_SUBTRACT:
            TOP1.as.number = TOP1.as.number - TOP0.as.number;
            --vm->stack_top;
            break;
        case INST_MULTIPLY:
            TOP1.as.number = TOP1.as.number * TOP0.as.number;
            --vm->stack_top;
            break;
        case INST_DIVIDE:
            TOP1.as.number = TOP1.as.number / TOP0.as.number;
            --vm->stack_top;
            break;
        case INST_POP:
            --vm->stack_top;
            break;
        case INST_CALL_EXTERNAL: {
            uint8_t external_num = *(vm->program->code.items + ip);
            ip += sizeof(external_num);

            if (vm->externals == NULL) {
                fprintf(stderr, "Fatal Error: Cannot resolve external function call `%02X` at position %zu: No external function definitions were provided to the vm.\n", external_num, ip_begin);
                exit(1);
            }

            if (external_num >= vm->externals->count) {
                fprintf(stderr, "Fatal Error: Cannot resolve external function call `%02X` at position %zu: Invalid external num.\n", external_num, ip_begin);
                exit(1);
            }

            vm->externals->items[external_num].callback(vm);
        }
        break;
        default:
            fprintf(stderr, "Fatal Error: Unknown instruction code '%02X' at position %zu.\n", instruction, ip_begin);
            exit(1);
        }

        if (vm->trace)
        {
            printf("Stack:");
            for (size_t i = 0; i < vm->stack_top; ++i)
            {
                printf(" ");
                dplv_print_value(vm, vm->stack[i]);
            }
            printf("\n");
        }
    }

#undef TOP1
#undef TOP0
}

DPL_Value dplv_peek(DPL_VirtualMachine *vm)
{
    return vm->stack[vm->stack_top - 1];
}