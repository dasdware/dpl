#include "vm.h"

#include "error.h"
#include "externals.h"
#include "math.h"

void dplv_init(DPL_VirtualMachine *vm, DPL_Program *program, struct DPL_ExternalFunctions *externals)
{
    vm->program = program;
    vm->externals = externals;

    if (vm->stack_capacity == 0)
    {
        vm->stack_capacity = 1024;
    }

    vm->stack = arena_alloc(&vm->memory, vm->stack_capacity * sizeof(*vm->stack));

    if (vm->callstack_capacity == 0) {
        vm->callstack_capacity = 256;
    }

    vm->callstack = arena_alloc(&vm->memory, vm->callstack_capacity * sizeof(*vm->callstack));

    st_init(&vm->strings);
}

void dplv_free(DPL_VirtualMachine *vm)
{
    st_free(&vm->strings);
    arena_free(&vm->memory);
}

void _dplv_push_callframe(DPL_VirtualMachine *vm, size_t return_ip) {
    if (vm->callstack_top >= vm->callstack_capacity)
    {
        DW_ERROR("Fatal Error: Callstack overflow in program execution.");
    }

    vm->callstack[vm->callstack_top].return_ip = return_ip;
    vm->callstack[vm->callstack_top].stack_top = vm->stack_top;
    ++vm->callstack_top;
}

DPL_CallFrame *_dplv_peek_callframe(DPL_VirtualMachine *vm) {
    if (vm->callstack_top == 0) {
        DW_ERROR("Fatal Error: Callstack underflow in program execution.");
    }

    return &vm->callstack[vm->callstack_top - 1];
}

void _dplv_pop_callframe(DPL_VirtualMachine *vm) {
    if (vm->callstack_top == 0) {
        DW_ERROR("Fatal Error: Callstack underflow in program execution.");
    }

    --vm->callstack_top;
}

void _dplv_trace_stack(DPL_VirtualMachine *vm) {
    printf("Stack:");
    for (size_t i = 0; i < vm->stack_top; ++i)
    {
        printf(" ");
        dpl_value_print(vm->stack[i]);
    }
    printf("\n");

}

void dplv_run(DPL_VirtualMachine *vm)
{
#define TOP0 (vm->stack[vm->stack_top - 1])
#define TOP1 (vm->stack[vm->stack_top - 2])

    _dplv_push_callframe(vm, 0);

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
                DW_ERROR("Fatal Error: Stack overflow in program execution.");
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
            if (vm->stack_top >= vm->stack_capacity)
            {
                DW_ERROR("Fatal Error: Stack overflow in program execution.");
            }

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
            if (TOP0.kind == VALUE_NUMBER && TOP1.kind == VALUE_NUMBER) {
                TOP1.as.number = TOP1.as.number + TOP0.as.number;
                --vm->stack_top;
            } else if (TOP0.kind == VALUE_STRING && TOP1.kind == VALUE_STRING) {
                Nob_String_View result = st_allocate_concat(&vm->strings, TOP1.as.string, TOP0.as.string);
                st_release(&vm->strings, TOP0.as.string);
                st_release(&vm->strings, TOP1.as.string);
                TOP1.as.string = result;
                --vm->stack_top;
            }
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
            if (TOP0.kind == VALUE_STRING) {
                st_release(&vm->strings, TOP0.as.string);
            }
            --vm->stack_top;
            break;
        case INST_CALL_EXTERNAL: {
            uint8_t external_num = *(vm->program->code.items + ip);
            ip += sizeof(external_num);

            if (vm->externals == NULL) {
                DW_ERROR("Fatal Error: Cannot resolve external function call `%02X` at position %zu: No external function definitions were provided to the vm.", external_num, ip_begin);
            }

            if (external_num >= vm->externals->count) {
                DW_ERROR("Fatal Error: Cannot resolve external function call `%02X` at position %zu: Invalid external num.", external_num, ip_begin);
            }

            vm->externals->items[external_num].callback(vm);
        }
        break;
        case INST_PUSH_LOCAL: {
            if (vm->stack_top >= vm->stack_capacity)
            {
                DW_ERROR("Fatal Error: Stack overflow in program execution.");
            }

            size_t scope_index = *(vm->program->code.items + ip);
            ip += sizeof(scope_index);

            vm->stack[vm->stack_top] = vm->stack[_dplv_peek_callframe(vm)->stack_top + scope_index];
            ++vm->stack_top;
        }
        break;
        case INST_STORE_LOCAL: {
            size_t scope_index = *(vm->program->code.items + ip);
            ip += sizeof(scope_index);

            vm->stack[_dplv_peek_callframe(vm)->stack_top + scope_index] = vm->stack[vm->stack_top - 1];
        }
        break;
        case INST_POP_SCOPE: {
            size_t scope_size = *(vm->program->code.items + ip);
            ip += sizeof(scope_size);

            DPL_Value result_value = vm->stack[vm->stack_top - 1];
            vm->stack_top -= scope_size;
            vm->stack[vm->stack_top - 1] = result_value;
        }
        break;
        default:
            printf("\n=======================================\n");
            _dplv_trace_stack(vm);
            DW_UNIMPLEMENTED_MSG("`%s` at position %zu.", dplp_inst_kind_name(instruction), ip_begin);
        }

        if (vm->trace) {
            _dplv_trace_stack(vm);
        }
    }

    _dplv_pop_callframe(vm);

#undef TOP1
#undef TOP0
}

DPL_Value dplv_peek(DPL_VirtualMachine *vm)
{
    return vm->stack[vm->stack_top - 1];
}

void dplv_replace_top(DPL_VirtualMachine *vm, DPL_Value value) {
    vm->stack[vm->stack_top - 1] = value;
}