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

DPL_Value dplv_reference(DPL_VirtualMachine* vm, DPL_Value value) {
    if (value.kind == VALUE_STRING) {
        return dpl_value_make_string(
                   st_allocate_sv(&vm->strings, value.as.string));
    }
    return value;
}

void dplv_release(DPL_VirtualMachine* vm, DPL_Value value) {
    if (value.kind == VALUE_STRING) {
        st_release(&vm->strings, value.as.string);
    }
}

void dplv_return(DPL_VirtualMachine* vm, size_t arity, DPL_Value value) {
    for (size_t i = vm->stack_top - arity; i < vm->stack_top; ++i) {
        dplv_release(vm, vm->stack[i]);
    }

    vm->stack_top -= (arity - 1);
    vm->stack[vm->stack_top - 1] = value;
}

void dplv_return_number(DPL_VirtualMachine* vm, size_t arity, double value) {
    dplv_return(vm, arity, dpl_value_make_number(value));
}

void dplv_return_string(DPL_VirtualMachine* vm, size_t arity, Nob_String_View value) {
    dplv_return(vm, arity, dpl_value_make_string(value));
}

void dplv_run(DPL_VirtualMachine *vm)
{
#define TOP0 (vm->stack[vm->stack_top - 1])
#define TOP1 (vm->stack[vm->stack_top - 2])

    _dplv_push_callframe(vm, 0);

    size_t ip = vm->program->entry;
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

            ++vm->stack_top;
            TOP0 = dpl_value_make_number(value);
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

            ++vm->stack_top;
            TOP0 = dpl_value_make_string(st_allocate_lstr(&vm->strings, data, length));
        }
        break;
        case INST_NEGATE:
            dplv_return_number(vm, 1, -TOP0.as.number);
            break;
        case INST_ADD:
            if (TOP0.kind == VALUE_NUMBER && TOP1.kind == VALUE_NUMBER) {
                dplv_return_number(vm, 2, TOP1.as.number + TOP0.as.number);
            } else if (TOP0.kind == VALUE_STRING && TOP1.kind == VALUE_STRING) {
                dplv_return_string(vm, 2, st_allocate_concat(&vm->strings, TOP1.as.string, TOP0.as.string));
            }
            break;
        case INST_SUBTRACT:
            dplv_return_number(vm, 2, TOP1.as.number - TOP0.as.number);
            break;
        case INST_MULTIPLY:
            dplv_return_number(vm, 2, TOP1.as.number * TOP0.as.number);
            break;
        case INST_DIVIDE:
            dplv_return_number(vm, 2, TOP1.as.number / TOP0.as.number);
            break;
        case INST_POP:
            dplv_release(vm, TOP0);
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

            size_t slot = _dplv_peek_callframe(vm)->stack_top + scope_index;

            ++vm->stack_top;
            TOP0 = dplv_reference(vm, vm->stack[slot]);
        }
        break;
        case INST_STORE_LOCAL: {
            size_t scope_index = *(vm->program->code.items + ip);
            ip += sizeof(scope_index);

            size_t slot = _dplv_peek_callframe(vm)->stack_top + scope_index;

            dplv_release(vm, vm->stack[slot]);
            vm->stack[slot] = dplv_reference(vm, TOP0);
        }
        break;
        case INST_POP_SCOPE: {
            size_t scope_size = *(vm->program->code.items + ip);
            ip += sizeof(scope_size);

            dplv_return(vm, scope_size + 1, dplv_reference(vm, TOP0));
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
