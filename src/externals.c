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
    printf("%f\n", dplv_peek(vm));
}

void dple_init(DPL_ExternalFunctions* externals)
{
    DPL_ExternalFunction* print = dple_add_by_name(externals, "print");
    nob_da_append(&print->argument_types, "number");
    print->return_type = "number";
    print->callback = _dple_print_callback;
}

void dple_free(DPL_ExternalFunctions *externals)
{
    nob_da_free(*externals);
}