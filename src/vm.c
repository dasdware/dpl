#include "vm.h"

void dplv_init(DPL_VirtualMachine *vm, DPL_Program *program)
{
    vm->program = program;

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
        default:
            fprintf(stderr, "Fatal Error: Unknown instruction code '%02X' at position %zu.\n", instruction, ip - 1);
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