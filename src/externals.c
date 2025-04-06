#ifdef DPL_LEAKCHECK
#include "stb_leakcheck.h"
#endif

#include "error.h"
#include "externals.h"
#include <dpl/vm/vm.h>

DPL_ExternalFunction *dple_add_by_name(DPL_ExternalFunctions *externals, const char *name)
{
    DPL_ExternalFunction external = {
        .name = name,
        .argument_types = 0,
        .return_type = NULL,
        .callback = NULL,
    };

    size_t index = da_size(*externals);
    da_add(*externals, external);
    return &((*externals)[index]);
}

void _dple_print_callback(DPL_VirtualMachine *vm)
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

void _dple_length_string_callback(DPL_VirtualMachine *vm)
{
    DPL_Value value = dplv_peek(vm);
    dplv_return_number(vm, 1, value.as.string.count);
}

void _dple_to_string_number_callback(DPL_VirtualMachine *vm)
{
    DPL_Value value = dplv_peek(vm);
    dplv_return_string(vm, 1,
                       mt_sv_allocate_cstr(&vm->stack_memory, dpl_value_format_number(value.as.number)));
}

void _dple_to_string_boolean_callback(DPL_VirtualMachine *vm)
{
    DPL_Value value = dplv_peek(vm);
    dplv_return_string(vm, 1,
                       mt_sv_allocate_cstr(&vm->stack_memory, dpl_value_format_boolean(value.as.boolean)));
    DW_UNUSED(vm);
}

void dple_init(DPL_ExternalFunctions *externals)
{
    DPL_ExternalFunction *print_number = dple_add_by_name(externals, "print");
    da_add(print_number->argument_types, "Number");
    print_number->return_type = "Number";
    print_number->callback = _dple_print_callback;

    DPL_ExternalFunction *print_string = dple_add_by_name(externals, "print");
    da_add(print_string->argument_types, "String");
    print_string->return_type = "String";
    print_string->callback = _dple_print_callback;

    DPL_ExternalFunction *print_boolean = dple_add_by_name(externals, "print");
    da_add(print_boolean->argument_types, "Boolean");
    print_boolean->return_type = "Boolean";
    print_boolean->callback = _dple_print_callback;

    DPL_ExternalFunction *length_string = dple_add_by_name(externals, "length");
    da_add(length_string->argument_types, "String");
    length_string->return_type = "Number";
    length_string->callback = _dple_length_string_callback;

    DPL_ExternalFunction *to_string_number = dple_add_by_name(externals, "toString");
    da_add(to_string_number->argument_types, "Number");
    to_string_number->return_type = "String";
    to_string_number->callback = _dple_to_string_number_callback;

    DPL_ExternalFunction *to_string_boolean = dple_add_by_name(externals, "toString");
    da_add(to_string_boolean->argument_types, "Boolean");
    to_string_boolean->return_type = "String";
    to_string_boolean->callback = _dple_to_string_boolean_callback;
}

void dple_free(DPL_ExternalFunctions *externals)
{
    for (size_t i = 0; i < da_size(*externals); ++i)
    {
        da_free((*externals)[i].argument_types);
    }
    da_free(*externals);
}
