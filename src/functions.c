#include "functions.h"

void dplf_init(DPL_Functions* functions, DPL_Types* types) {
    dplf_register(functions, nob_sv_from_cstr("negate"), types->unary_handle);

    dplf_register(functions, nob_sv_from_cstr("add"), types->binary_handle);
    dplf_register(functions, nob_sv_from_cstr("subtract"), types->binary_handle);
    dplf_register(functions, nob_sv_from_cstr("multiply"), types->binary_handle);
    dplf_register(functions, nob_sv_from_cstr("divide"), types->binary_handle);
}

void dplf_free(DPL_Functions* functions) {
    nob_da_free(*functions);
}

DPL_Function_Handle dplf_register(DPL_Functions* functions, Nob_String_View name,
                                  DPL_Type_Handle type)
{
    DPL_Function function = {
        .handle = functions->count + 1,
        .name = name,
        .type = type,
    };
    nob_da_append(functions, function);

    return function.handle;
}

DPL_Function* dplf_find_by_handle(DPL_Functions *functions, DPL_Function_Handle handle)
{
    for (size_t i = 0;  i < functions->count; ++i) {
        if (functions->items[i].handle == handle) {
            return &functions->items[i];
        }
    }
    return 0;
}

DPL_Function* dplf_find_by_signature1(DPL_Types* types, DPL_Functions *functions,
                                      Nob_String_View name, DPL_Type_Handle arg0)
{
    for (size_t i = 0; i < functions->count; ++i) {
        DPL_Function* function = &functions->items[i];
        if (nob_sv_eq(function->name, name)) {
            DPL_Type* function_type = dplt_find_by_handle(types, function->type);
            if (function_type->kind == TYPE_FUNCTION
                    && function_type->as.function.arguments.count == 1
                    && function_type->as.function.arguments.items[0] == arg0
               ) {
                return function;
            }
        }
    }

    return 0;
}

DPL_Function* dplf_find_by_signature2(DPL_Types* types, DPL_Functions *functions,
                                      Nob_String_View name, DPL_Type_Handle arg0, DPL_Type_Handle arg1)
{
    for (size_t i = 0; i < functions->count; ++i) {
        DPL_Function* function = &functions->items[i];
        if (nob_sv_eq(function->name, name)) {
            DPL_Type* function_type = dplt_find_by_handle(types, function->type);
            if (function_type->kind == TYPE_FUNCTION
                    && function_type->as.function.arguments.count == 2
                    && function_type->as.function.arguments.items[0] == arg0
                    && function_type->as.function.arguments.items[1] == arg1
               ) {
                return function;
            }
        }
    }

    return 0;
}

void dplf_print(FILE* out, DPL_Types* types, DPL_Function* function) {
    if (!function) {
        fprintf(out, "<unknown function>");
        return;
    }

    fprintf(out, SV_Fmt, SV_Arg(function->name));
    dplt_print_meta_function(out, types, dplt_find_by_handle(types, function->type));
}