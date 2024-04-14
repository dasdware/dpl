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

void _dple_print_number_callback(DPL_VirtualMachine* vm)
{
    printf("%f\n", dplv_peek(vm).as.number);
}

void _dple_print_string_callback(DPL_VirtualMachine* vm)
{
    //TODO: Support for printing strings
    (void) vm;
}

void dple_init(DPL_ExternalFunctions* externals)
{
    DPL_ExternalFunction* print_number = dple_add_by_name(externals, "print");
    nob_da_append(&print_number->argument_types, "number");
    print_number->return_type = "number";
    print_number->callback = _dple_print_number_callback;

    DPL_ExternalFunction* print_string = dple_add_by_name(externals, "print");
    nob_da_append(&print_string->argument_types, "string");
    print_string->return_type = "string";
    print_string->callback = _dple_print_string_callback;
}

void dple_free(DPL_ExternalFunctions *externals)
{
    nob_da_free(*externals);
}