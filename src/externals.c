#include "error.h"
#include "externals.h"

DPL_ExternalFunction* dple_add_by_name(DPL_ExternalFunctions* externals, const char* name)
{
    DPL_ExternalFunction external = {
        .name = name,
        .argument_types = {0},
        .return_type = NULL,
        .callback = NULL,
    };

    size_t index = externals->count;
    nob_da_append(externals, external);
    return &externals->items[index];
}

void _dple_print_callback(DPL_VirtualMachine* vm)
{
    DPL_Value value = dplv_peek(vm);
    switch (value.kind) {
    case VALUE_NUMBER:
        printf("%s", dpl_value_format_number(value));
        break;
    case VALUE_STRING:
        printf(SV_Fmt, SV_Arg(value.as.string));
        break;
    default:
        DW_ERROR("ERROR: `print` function callback cannot print values of kind `%s`.", dpl_value_kind_name(value.kind));
    }
}

void _dple_length_string_callback(DPL_VirtualMachine* vm) {
    DPL_Value value = dplv_peek(vm);
    dplv_return_number(vm, 1, value.as.string.count);
}

void _dple_to_string_number_callback(DPL_VirtualMachine* vm) {
    DPL_Value value = dplv_peek(vm);
    dplv_return_string(vm, 1,
                       st_allocate_cstr(&vm->strings, dpl_value_format_number(value)));
}


void dple_init(DPL_ExternalFunctions* externals)
{
    DPL_ExternalFunction* print_number = dple_add_by_name(externals, "print");
    nob_da_append(&print_number->argument_types, "number");
    print_number->return_type = "number";
    print_number->callback = _dple_print_callback;

    DPL_ExternalFunction* print_string = dple_add_by_name(externals, "print");
    nob_da_append(&print_string->argument_types, "string");
    print_string->return_type = "string";
    print_string->callback = _dple_print_callback;

    DPL_ExternalFunction* length_string = dple_add_by_name(externals, "length");
    nob_da_append(&length_string->argument_types, "string");
    length_string->return_type = "number";
    length_string->callback = _dple_length_string_callback;

    DPL_ExternalFunction* to_string_number = dple_add_by_name(externals, "to_string");
    nob_da_append(&to_string_number->argument_types, "number");
    to_string_number->return_type = "string";
    to_string_number->callback = _dple_to_string_number_callback;
}

void dple_free(DPL_ExternalFunctions *externals)
{
    nob_da_free(*externals);
}