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
        printf("%f", value.as.number);
        break;
    case VALUE_STRING:
        printf("%s", st_get(&vm->strings, value.as.string));
        break;
    default:
        fprintf(stderr, "ERROR: `print` function callback cannot print values of kind `%s`.", dplv_value_kind_name(value.kind));
        exit(1);
    }
}

void _dple_length_string_callback(DPL_VirtualMachine* vm) {
    DPL_Value value = dplv_peek(vm);

    double length = st_length(&vm->strings, value.as.string);
    st_release(&vm->strings, value.as.string);

    dplv_replace_top(vm, dplv_number(length));
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

}

void dple_free(DPL_ExternalFunctions *externals)
{
    nob_da_free(*externals);
}