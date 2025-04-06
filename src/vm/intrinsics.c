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

const DPL_Intrinsic_Callback INTRINSIC_CALLBACKS[COUNT_INTRINSICS] = {
    [INTRINSIC_NUMBER_ITERATOR] = dpl_vm_intrinsic_number_iterator,
    [INTRINSIC_NUMBER_ITERATOR_NEXT] = dpl_vm_intrinsic_number_iterator_next,
};

static_assert(COUNT_INTRINSICS == 2,
              "Count of intrinsic kinds has changed, please update intrinsic kind names map.");

void dpl_vm_call_intrinsic(DPL_VirtualMachine *vm, DPL_Intrinsic_Kind kind)
{
    if (kind >= COUNT_INTRINSICS)
    {
        DW_ERROR("Invalid intrinsic kind %u.", kind);
    }
    INTRINSIC_CALLBACKS[kind](vm);
}