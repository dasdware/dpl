#include "vm.h"
#include "externals.h"

void dplv_init(DPL_VirtualMachine *vm, DPL_Program *program, struct DPL_ExternalFunctions *externals)
{
    vm->program = program;
    vm->externals = externals;

    if (vm->stack_capacity == 0)
    {
        vm->stack_capacity = 1024;
    }

    vm->stack = arena_alloc(&vm->memory, vm->stack_capacity * sizeof(DPL_Value));
}

void dplv_free(DPL_VirtualMachine *vm)
{
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
            vm->stack[vm->stack_top] = value;
            ++vm->stack_top;
        }
        break;
        case INST_NEGATE:
            TOP0 = -TOP0;
            break;
        case INST_ADD:
            TOP1 = TOP1 + TOP0;
            --vm->stack_top;
            break;
        case INST_SUBTRACT:
            TOP1 = TOP1 - TOP0;
            --vm->stack_top;
            break;
        case INST_MULTIPLY:
            TOP1 = TOP1 * TOP0;
            --vm->stack_top;
            break;
        case INST_DIVIDE:
            TOP1 = TOP1 / TOP0;
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

        if (vm->debug)
        {
            printf("Stack:");
            for (size_t i = 0; i < vm->stack_top; ++i)
            {
                printf(" [ %f ]", vm->stack[i]);
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