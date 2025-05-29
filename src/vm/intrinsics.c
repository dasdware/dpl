#include <dpl/vm/intrinsics.h>
#include <error.h>

typedef void (*DPL_Intrinsic_Callback)(DPL_VirtualMachine *);

static DPL_Value dpl_vm_intrinsic_make_object(DPL_VirtualMachine *vm, size_t field_count, DPL_Value *fields)
{
    size_t object_size = field_count * sizeof(DPL_Value);

    DW_MemoryTable_Item *object = mt_allocate(&vm->stack_memory, object_size);
    memcpy(object->data, fields, object_size);

    return dpl_value_make_object(object);
}

void dpl_vm_intrinsic_boolean_tostring(DPL_VirtualMachine *vm)
{
    // function toString(Boolean): String :=
    //   <native>;
    DPL_Value value = dplv_peek(vm);
    dplv_return_string(vm, 1,
                       mt_sv_allocate_cstr(&vm->stack_memory, dpl_value_format_boolean(value.as.boolean)));
}

void dpl_vm_intrinsic_number_tostring(DPL_VirtualMachine *vm)
{
    // function toString(Number): String :=
    //   <native>;
    DPL_Value value = dplv_peek(vm);
    dplv_return_string(vm, 1,
                       mt_sv_allocate_cstr(&vm->stack_memory, dpl_value_format_number(value.as.number)));
}

static void dpl_vm_intrinsic_number_iterator(DPL_VirtualMachine *vm)
{
    // function iterator(range: [from: Number, to: Number]): RangeIterator :=
    //   [ current := range.from, finished := range.from >= range.to, to := range.to ];
    DPL_Value range = dplv_peek(vm);
    DPL_Value from = dpl_value_object_get_field(range.as.object, 0);
    DPL_Value to = dpl_value_object_get_field(range.as.object, 1);

    dplv_return(
        vm,
        1,
        dpl_vm_intrinsic_make_object(
            vm,
            DPL_VALUES(from, dpl_value_make_boolean(from.as.number > to.as.number), to)));
}

static void dpl_vm_intrinsic_number_iterator_next(DPL_VirtualMachine *vm)
{
    // function next(iterator: [current: Number, finished: Boolean, to: Number]): [current: Number, finished: Boolean, to: Number] := {
    //     var current := iterator.current + 1;
    //     [ ..iterator, finished := current > iterator.to, current ]
    // };

    DPL_Value iterator = dplv_peek(vm);

    DPL_Value current = dpl_value_object_get_field(iterator.as.object, 0);
    DPL_Value to = dpl_value_object_get_field(iterator.as.object, 2);

    double next = current.as.number + 1;
    dplv_return(
        vm,
        1,
        dpl_vm_intrinsic_make_object(
            vm,
            DPL_VALUES(dpl_value_make_number(next), dpl_value_make_boolean(next > to.as.number), to)));
}

void dpl_vm_intrinsic_string_length(DPL_VirtualMachine *vm)
{
    // function length(String): Number :=
    //   <native>;
    DPL_Value value = dplv_peek(vm);
    dplv_return_number(vm, 1, value.as.string.count);
}

void dpl_vm_intrinsic_print(DPL_VirtualMachine *vm)
{
    DPL_Value value = dplv_peek(vm);
    switch (value.kind)
    {
    case VALUE_NUMBER:
        printf("%s", dpl_value_format_number(value.as.number));
        break;
    case VALUE_STRING:
        printf(SV_Fmt, SV_Arg(value.as.string));
        break;
    case VALUE_BOOLEAN:
        printf("%s", dpl_value_format_boolean(value.as.boolean));
        break;
    default:
        DW_ERROR("ERROR: `print` function callback cannot print values of kind `%s`.", dpl_value_kind_name(value.kind));
    }
}

void dpl_vm_intrinsic_array_length(DPL_VirtualMachine *vm)
{
    // function length([T]): Number :=
    //   <native>;
    DPL_Value value = dplv_peek(vm);
    dplv_return_number(vm, 1, dpl_value_array_element_count(value.as.array));
}

void dpl_vm_intrinsic_array_element(DPL_VirtualMachine *vm)
{
    // function element([T], Number): T :=
    //   <native>
    size_t index = dplv_peek(vm).as.number;
    DW_MemoryTable_Item *array = dplv_peekn(vm, 2).as.array;
    size_t array_size = dpl_value_array_element_count(array);

    if (index >= array_size)
    {
        DW_ERROR("Array index out of bounds (size: %zu, index: %zu).", array_size, index);
    }

    DPL_Value result = dpl_value_array_get_element(array, index);
    dplv_return(vm, 2, result);
}

void dpl_vm_intrinsic_array_iterator(DPL_VirtualMachine *vm)
{
    // function iterator(array: [T]): Iterator<T>
    //     := $[
    //         current := if (array.length() > 0) array[0] else none,
    //         finished := array.length() == 0,
    //         index := 0,
    //         array := array
    //     ];

    DPL_Value array = dplv_peek(vm);

    size_t count = dpl_value_array_element_count(array.as.array);
    DPL_Value iterator = dpl_vm_intrinsic_make_object(
        vm,
        DPL_VALUES(
            dplv_reference(vm, array),
            (count > 0) ? dpl_value_array_get_element(array.as.array, 0) : dpl_value_make_number(0),
            dpl_value_make_boolean(count == 0),
            dpl_value_make_number(0)));

    dplv_return(vm, 1, iterator);
}

void dpl_vm_intrinsic_arrayiterator_next(DPL_VirtualMachine *vm)
{
    // function next(it: NumberArrayIterator): NumberArrayIterator
    //     := {
    //         var index := it.index + 1;
    //         var finished := index >= it.array.length();
    //         if (finished)
    //             $[finished, current := -1, index, array := it.array]
    //         else
    //             $[finished, current := it.array[index], index, array := it.array]
    //     };

    DPL_Value it = dplv_peek(vm);

    DPL_Value array = dpl_value_object_get_field(it.as.object, 0);
    size_t count = dpl_value_array_element_count(array.as.array);

    DPL_Value index = dpl_value_object_get_field(it.as.object, 3);
    size_t next_index = index.as.number + 1;

    DPL_Value next_it = dpl_vm_intrinsic_make_object(
        vm,
        DPL_VALUES(
            dplv_reference(vm, array),
            (next_index < count) ? dpl_value_array_get_element(array.as.array, next_index) : dpl_value_make_number(0),
            dpl_value_make_boolean(next_index >= count),
            dpl_value_make_number(next_index)));

    dplv_return(vm, 1, next_it);
}

const DPL_Intrinsic_Callback INTRINSIC_CALLBACKS[COUNT_INTRINSICS] = {
    [INTRINSIC_BOOLEAN_PRINT] = dpl_vm_intrinsic_print,
    [INTRINSIC_BOOLEAN_TOSTRING] = dpl_vm_intrinsic_boolean_tostring,

    [INTRINSIC_NUMBER_PRINT] = dpl_vm_intrinsic_print,
    [INTRINSIC_NUMBER_TOSTRING] = dpl_vm_intrinsic_number_tostring,
    [INTRINSIC_NUMBERITERATOR_NEXT] = dpl_vm_intrinsic_number_iterator_next,
    [INTRINSIC_NUMBERRANGE_ITERATOR] = dpl_vm_intrinsic_number_iterator,

    [INTRINSIC_STRING_LENGTH] = dpl_vm_intrinsic_string_length,
    [INTRINSIC_STRING_PRINT] = dpl_vm_intrinsic_print,

    [INTRINSIC_ARRAY_LENGTH] = dpl_vm_intrinsic_array_length,
    [INTRINSIC_ARRAY_ELEMENT] = dpl_vm_intrinsic_array_element,
    [INTRINSIC_ARRAY_ITERATOR] = dpl_vm_intrinsic_array_iterator,
    [INTRINSIC_ARRAYITERATOR_NEXT] = dpl_vm_intrinsic_arrayiterator_next,
};

static_assert(COUNT_INTRINSICS == 12,
              "Count of intrinsic kinds has changed, please update intrinsic kind names map.");

void dpl_vm_call_intrinsic(DPL_VirtualMachine *vm, DPL_Intrinsic_Kind kind)
{
    if (kind >= COUNT_INTRINSICS)
    {
        DW_ERROR("Invalid intrinsic kind %u.", kind);
    }
    INTRINSIC_CALLBACKS[kind](vm);
}