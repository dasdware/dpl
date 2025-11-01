#ifdef DPL_LEAKCHECK
#include "stb_leakcheck.h"
#endif

#include <dpl/vm/intrinsics.h>
#include <dpl/vm/vm.h>

#include "error.h"
#include "math.h"

int dplv_print(void* context, char const *str, ...)
{
    DW_UNUSED(context);

    va_list args;
    va_start(args, str);
    int n = vprintf(str, args);
    va_end(args);
    return n;
}

void dplv_init(DPL_VirtualMachine *vm, DPL_Program *program)
{
    vm->print_callback = dplv_print;
    vm->program = program;

    if (vm->stack_capacity == 0)
    {
        vm->stack_capacity = 1024;
    }

    vm->stack = arena_alloc(&vm->memory, vm->stack_capacity * sizeof(*vm->stack));

    if (vm->callstack_capacity == 0)
    {
        vm->callstack_capacity = 256;
    }

    vm->callstack = arena_alloc(&vm->memory, vm->callstack_capacity * sizeof(*vm->callstack));
}

void dplv_free(DPL_VirtualMachine *vm)
{
    arena_free(&vm->memory);
}

void _dplv_push_callframe(DPL_VirtualMachine *vm, size_t arity, size_t return_ip)
{
    if (vm->callstack_top >= vm->callstack_capacity)
    {
        DW_ERROR("Fatal Error: Callstack overflow in program execution.");
    }

    vm->callstack[vm->callstack_top].return_ip = return_ip;
    vm->callstack[vm->callstack_top].arity = arity;
    vm->callstack[vm->callstack_top].stack_top = vm->stack_top - arity;
    ++vm->callstack_top;
}

DPL_CallFrame *_dplv_peek_callframe(DPL_VirtualMachine *vm)
{
    if (vm->callstack_top == 0)
    {
        DW_ERROR("Fatal Error: Callstack underflow in program execution.");
    }

    return &vm->callstack[vm->callstack_top - 1];
}

void _dplv_pop_callframe(DPL_VirtualMachine *vm)
{
    if (vm->callstack_top == 0)
    {
        DW_ERROR("Fatal Error: Callstack underflow in program execution.");
    }

    --vm->callstack_top;
}

void _dplv_trace_stack(DPL_VirtualMachine *vm)
{
    printf("<");
    for (size_t i = 0; i < vm->stack_top; ++i)
    {
        printf(" ");
        dpl_value_print(vm->stack[i]);
    }
    printf(" >");
}

DPL_Value dplv_reference(DPL_VirtualMachine *vm, DPL_Value value)
{
    if (value.kind == VALUE_STRING)
    {
        mt_sv_reference(&vm->stack_memory, value.as.string);
    }
    else if (value.kind == VALUE_OBJECT)
    {
        mt_reference(&vm->stack_memory, value.as.object);
    }
    else if (value.kind == VALUE_ARRAY)
    {
        mt_reference(&vm->stack_memory, value.as.array);
    }
    return value;
}

void dplv_release(DPL_VirtualMachine *vm, DPL_Value value)
{
    if (value.kind == VALUE_STRING)
    {
        mt_sv_release(&vm->stack_memory, value.as.string);
    }
    else if (value.kind == VALUE_OBJECT)
    {
        if (mt_will_release(&vm->stack_memory, value.as.object))
        {
            for (size_t i = 0; i < dpl_value_object_field_count(value.as.object); ++i)
            {
                dplv_release(vm, dpl_value_object_get_field(value.as.object, i));
            }
        }
        mt_release(&vm->stack_memory, value.as.object);
    }
    else if (value.kind == VALUE_ARRAY)
    {
        if (mt_will_release(&vm->stack_memory, value.as.array))
        {
            for (size_t i = 0; i < dpl_value_array_element_count(value.as.array); ++i)
            {
                dplv_release(vm, dpl_value_array_get_element(value.as.array, i));
            }
        }
        mt_release(&vm->stack_memory, value.as.array);
    }
}

void dplv_return(DPL_VirtualMachine *vm, size_t arity, DPL_Value value)
{
    for (size_t i = vm->stack_top - arity; i < vm->stack_top; ++i)
    {
        dplv_release(vm, vm->stack[i]);
    }

    vm->stack_top -= (arity - 1);
    vm->stack[vm->stack_top - 1] = value;
}

void dplv_return_number(DPL_VirtualMachine *vm, size_t arity, double value)
{
    dplv_return(vm, arity, dpl_value_make_number(value));
}

void dplv_return_string(DPL_VirtualMachine *vm, size_t arity, Nob_String_View value)
{
    dplv_return(vm, arity, dpl_value_make_string(value));
}

void dplv_return_boolean(DPL_VirtualMachine *vm, size_t arity, bool value)
{
    dplv_return(vm, arity, dpl_value_make_boolean(value));
}

void dplv_run_begin(DPL_VirtualMachine *vm)
{
    mt_init(&vm->stack_memory);
    _dplv_push_callframe(vm, 0, 0);

    vm->program_stream = (DW_ByteStream) {
        .buffer = vm->program->code,
        .position = vm->program->entry,
    };
    vm->constants_stream = (DW_ByteStream) {
        .buffer = vm->program->constants,
        .position = 0,
    };
}

void dplv_run_end(DPL_VirtualMachine *vm)
{
    _dplv_pop_callframe(vm);
    mt_free(&vm->stack_memory);
}

void dplv_run_step(DPL_VirtualMachine *vm)
{
#define TOP0 (vm->stack[vm->stack_top - 1])
#define TOP1 (vm->stack[vm->stack_top - 2])
    if (vm->trace)
    {
        DW_ByteStream trace_program = vm->program_stream;
        dplp_print_stream_instruction(&trace_program, &vm->constants_stream);
        printf("    :: ");
        _dplv_trace_stack(vm);
    }

    size_t ip_begin = vm->program_stream.position;

    DPL_Instruction_Kind instruction = bs_read_u8(&vm->program_stream);
    switch (instruction)
    {
    case INST_NOOP:
        break;
    case INST_PUSH_NUMBER:
    {
        if (vm->stack_top >= vm->stack_capacity)
        {
            DW_ERROR("Fatal Error: Stack overflow in program execution.");
        }

        double value = bs_read_f64(&vm->program_stream);

        ++vm->stack_top;
        TOP0 = dpl_value_make_number(value);
    }
    break;
    case INST_PUSH_STRING:
    {
        if (vm->stack_top >= vm->stack_capacity)
        {
            DW_ERROR("Fatal Error: Stack overflow in program execution.");
        }

        size_t offset = bs_read_u64(&vm->program_stream);

        Nob_String_View value = bb_read_sv(vm->program->constants, offset);

        ++vm->stack_top;
        TOP0 = dpl_value_make_string(mt_sv_allocate_sv(&vm->stack_memory, value));
    }
    break;
    case INST_PUSH_BOOLEAN:
    {
        if (vm->stack_top >= vm->stack_capacity)
        {
            DW_ERROR("Fatal Error: Stack overflow in program execution.");
        }

        size_t value = bs_read_u8(&vm->program_stream);

        ++vm->stack_top;
        TOP0 = dpl_value_make_boolean(value == 1);
    }
    break;
    case INST_NEGATE:
        dplv_return_number(vm, 1, -TOP0.as.number);
        break;
    case INST_NOT:
        dplv_return_boolean(vm, 1, !TOP0.as.boolean);
        break;
    case INST_ADD:
        if (TOP0.kind == VALUE_NUMBER && TOP1.kind == VALUE_NUMBER)
        {
            dplv_return_number(vm, 2, TOP1.as.number + TOP0.as.number);
        }
        else if (TOP0.kind == VALUE_STRING && TOP1.kind == VALUE_STRING)
        {
            dplv_return_string(vm, 2, mt_sv_allocate_concat(&vm->stack_memory, TOP1.as.string, TOP0.as.string));
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
    case INST_LESS:
        dplv_return_boolean(vm, 2, dpl_value_compare_numbers(TOP1.as.number, TOP0.as.number) < 0);
        break;
    case INST_LESS_EQUAL:
        dplv_return_boolean(vm, 2, dpl_value_compare_numbers(TOP1.as.number, TOP0.as.number) <= 0);
        break;
    case INST_GREATER:
        dplv_return_boolean(vm, 2, dpl_value_compare_numbers(TOP1.as.number, TOP0.as.number) > 0);
        break;
    case INST_GREATER_EQUAL:
        dplv_return_boolean(vm, 2, dpl_value_compare_numbers(TOP1.as.number, TOP0.as.number) >= 0);
        break;
    case INST_EQUAL:
        if (TOP0.kind == VALUE_NUMBER && TOP1.kind == VALUE_NUMBER)
        {
            dplv_return_boolean(vm, 2, dpl_value_compare_numbers(TOP1.as.number, TOP0.as.number) == 0);
        }
        else if (TOP0.kind == VALUE_STRING && TOP1.kind == VALUE_STRING)
        {
            dplv_return_boolean(vm, 2, dpl_value_string_equals(TOP0.as.string, TOP1.as.string));
        }
        else if (TOP0.kind == VALUE_BOOLEAN && TOP1.kind == VALUE_BOOLEAN)
        {
            dplv_return_boolean(vm, 2, TOP0.as.boolean == TOP1.as.boolean);
        }
        break;
    case INST_NOT_EQUAL:
        if (TOP0.kind == VALUE_NUMBER && TOP1.kind == VALUE_NUMBER)
        {
            dplv_return_boolean(vm, 2, dpl_value_compare_numbers(TOP1.as.number, TOP0.as.number) != 0);
        }
        else if (TOP0.kind == VALUE_STRING && TOP1.kind == VALUE_STRING)
        {
            dplv_return_boolean(vm, 2, !dpl_value_string_equals(TOP0.as.string, TOP1.as.string));
        }
        else if (TOP0.kind == VALUE_BOOLEAN && TOP1.kind == VALUE_BOOLEAN)
        {
            dplv_return_boolean(vm, 2, TOP0.as.boolean != TOP1.as.boolean);
        }
        break;
    case INST_POP:
        if (vm->stack_top == 0)
        {
            DW_ERROR("Fatal Error: Stack underflow in program execution.");
        }
        dplv_release(vm, TOP0);
        --vm->stack_top;
        break;
    case INST_CALL_INTRINSIC:
    {
        DPL_Intrinsic_Kind intrinsic = bs_read_u8(&vm->program_stream);
        dpl_vm_call_intrinsic(vm, intrinsic);
    }
    break;
    case INST_PUSH_LOCAL:
    {
        if (vm->stack_top >= vm->stack_capacity)
        {
            DW_ERROR("Fatal Error: Stack overflow in program execution.");
        }

        size_t scope_index = bs_read_u64(&vm->program_stream);
        size_t slot = _dplv_peek_callframe(vm)->stack_top + scope_index;

        ++vm->stack_top;
        TOP0 = dplv_reference(vm, vm->stack[slot]);
    }
    break;
    case INST_STORE_LOCAL:
    {
        size_t scope_index = bs_read_u64(&vm->program_stream);
        size_t slot = _dplv_peek_callframe(vm)->stack_top + scope_index;

        dplv_release(vm, vm->stack[slot]);
        vm->stack[slot] = dplv_reference(vm, TOP0);
    }
    break;
    case INST_POP_SCOPE:
    {
        size_t scope_size = bs_read_u64(&vm->program_stream);

        dplv_return(vm, scope_size + 1, dplv_reference(vm, TOP0));
    }
    break;
    case INST_CALL_USER:
    {
        uint8_t arity = bs_read_u8(&vm->program_stream);
        size_t begin_ip = bs_read_u64(&vm->program_stream);

        _dplv_push_callframe(vm, arity, vm->program_stream.position);

        vm->program_stream.position = begin_ip;
    };
    break;
    case INST_RETURN:
    {
        DPL_CallFrame *frame = _dplv_peek_callframe(vm);
        dplv_return(vm, frame->arity + 1, dplv_reference(vm, TOP0));
        vm->program_stream.position = frame->return_ip;

        _dplv_pop_callframe(vm);
    };
    break;
    case INST_JUMP:
    {
        uint16_t jump = bs_read_u16(&vm->program_stream);
        vm->program_stream.position += jump;
    }
    break;
    case INST_JUMP_IF_FALSE:
    {
        uint16_t jump = bs_read_u16(&vm->program_stream);
        if (!TOP0.as.boolean)
        {
            vm->program_stream.position += jump;
        }
    }
    break;
    case INST_JUMP_IF_TRUE:
    {
        uint16_t jump = bs_read_u16(&vm->program_stream);
        if (TOP0.as.boolean)
        {
            vm->program_stream.position += jump;
        }
    }
    break;
    case INST_JUMP_LOOP:
    {
        uint16_t jump = bs_read_u16(&vm->program_stream);
        vm->program_stream.position -= jump;
    }
    break;
    case INST_CREATE_OBJECT:
    {
        uint8_t field_count = bs_read_u8(&vm->program_stream);

        size_t object_size = field_count * sizeof(DPL_Value);

        DW_MemoryTable_Item *object = mt_allocate(&vm->stack_memory, object_size);
        memcpy(object->data, &vm->stack[vm->stack_top - field_count], object_size);

        vm->stack_top -= (field_count - 1);
        TOP0 = dpl_value_make_object(object);
    }
    break;
    case INST_LOAD_FIELD:
    {
        uint8_t field_index = bs_read_u8(&vm->program_stream);

        DPL_Value field_value = dplv_reference(vm, dpl_value_object_get_field(TOP0.as.object, field_index));
        dplv_release(vm, TOP0);
        TOP0 = field_value;
    }
    break;
    case INST_INTERPOLATION:
    {
        uint8_t count = bs_read_u8(&vm->program_stream);

        Nob_String_Builder result = {0};
        for (size_t i = vm->stack_top - count; i < vm->stack_top; ++i)
        {
            nob_sb_append_buf(&result, vm->stack[i].as.string.data, vm->stack[i].as.string.count);
        }
        nob_sb_append_null(&result);

        ++vm->stack_top;
        TOP0 = dpl_value_make_string(mt_sv_allocate_cstr(&vm->stack_memory, result.items));

        nob_sb_free(result);

        dplv_return(vm, count + 1, dplv_reference(vm, TOP0));
    }
    break;
    case INST_BEGIN_ARRAY:
    {
        if (vm->stack_top >= vm->stack_capacity)
        {
            DW_ERROR("Fatal Error: Stack overflow in program execution.");
        }

        ++vm->stack_top;
        TOP0 = dpl_value_make_array_slot();
    }
    break;
    case INST_END_ARRAY:
    {
        for (size_t i = vm->stack_top; i > 0; --i)
        {
            if (vm->stack[i - 1].kind == VALUE_ARRAY && vm->stack[i - 1].as.array == NULL)
            {
                size_t element_count = vm->stack_top - i;
                vm->stack[i - 1].as.array = mt_allocate_data(&vm->stack_memory, &vm->stack[i], sizeof(DPL_Value) * element_count);
                vm->stack_top -= element_count;
            }
        }
    }
    break;
    case INST_SPREAD:
    {
        DPL_Value value = TOP0;

        size_t count = dpl_value_array_element_count(value.as.array);
        if ((vm->stack_top + count - 1) >= vm->stack_capacity)
        {
            DW_ERROR("Fatal Error: Stack overflow in program execution.");
        }

        for (size_t i = 0; i < count; ++i)
        {
            vm->stack[vm->stack_top - 1 + i] = dplv_reference(
                vm, dpl_value_array_get_element(value.as.array, i));
        }
        vm->stack_top += count - 1;

        dplv_release(vm, value);
    }

    break;
    default:
        printf("\n=======================================\n");
        _dplv_trace_stack(vm);
        printf("\n");
        DW_UNIMPLEMENTED_MSG("`%s` at position %zu.", dplp_inst_kind_name(instruction), ip_begin);
    }

    if (vm->trace)
    {
        printf("\n    :: ");
        _dplv_trace_stack(vm);
        printf(" [%04zu]\n", vm->program_stream.position);
        getc(stdin);
    }
#undef TOP1
#undef TOP0
}

void dplv_run(DPL_VirtualMachine *vm)
{
    dplv_run_begin(vm);

    while (!bs_at_end(&vm->program_stream))
    {
        dplv_run_step(vm);
    }

    dplv_run_end(vm);
}


DPL_Value dplv_peek(DPL_VirtualMachine *vm)
{
    return dplv_peekn(vm, 1);
}

DPL_Value dplv_peekn(DPL_VirtualMachine *vm, size_t n)
{
    if (vm->stack_top < n)
    {
        DW_ERROR("Fatal Error: Stack underflow in program execution.");
    }

    return vm->stack[vm->stack_top - n];
}
