#include "types.h"

size_t
dplt_hash(Nob_String_View sv)
{
    size_t hash = 5381;
    while (sv.count > 0) {
        int c = *sv.data;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        sv.data++;
        sv.count--;
    }

    return hash;
}

DPL_Type_Handle dplt_add(DPL_Types* types, DPL_Type type) {
    size_t index = types->count;
    nob_da_append(types, type);

    return index;
}

DPL_Type_Handle dplt_register_by_name(DPL_Types* types, Nob_String_View name)
{
    DPL_Type type = {
        .name = name,
        .handle = types->count + 1,
        .hash = dplt_hash(name),
    };
    dplt_add(types, type);
    return type.handle;
}

DPL_Type_Handle dplt_register(DPL_Types* types, DPL_Type type)
{
    type.handle = types->count + 1;
    type.hash = dplt_hash(type.name);
    dplt_add(types, type);
    return type.handle;
}

void dplt_init(DPL_Types* types)
{
    types->number_handle = dplt_register_by_name(types, nob_sv_from_cstr("number"));

    {
        DPL_Type unary = {0};
        unary.name = nob_sv_from_cstr("unary");
        unary.kind = TYPE_FUNCTION;
        dplt_add_handle(&unary.as.function.arguments, types->number_handle);
        unary.as.function.returns = types->number_handle;
        types->unary_handle = dplt_register(types, unary);
    }

    {
        DPL_Type binary = {0};
        binary.name = nob_sv_from_cstr("binary");
        binary.kind = TYPE_FUNCTION;
        dplt_add_handle(&binary.as.function.arguments, types->number_handle);
        dplt_add_handle(&binary.as.function.arguments, types->number_handle);
        binary.as.function.returns = types->number_handle;
        types->binary_handle = dplt_register(types, binary);
    }
}

void dplt_free(DPL_Types* types) {
    nob_da_free(*types);
}

DPL_Type* dplt_find_by_handle(DPL_Types *types, DPL_Type_Handle handle)
{
    for (size_t i = 0;  i < types->count; ++i) {
        if (types->items[i].handle == handle) {
            return &types->items[i];
        }
    }
    return 0;
}

DPL_Type* dplt_find_by_name(DPL_Types *types, Nob_String_View name)
{
    for (size_t i = 0;  i < types->count; ++i) {
        if (nob_sv_eq(types->items[i].name, name)) {
            return &types->items[i];
        }
    }
    return 0;
}

void dplt_add_handle(DPL_Type_Handles* handles, DPL_Type_Handle handle)
{
    nob_da_append(handles, handle);
}

bool dplt_add_handle_by_name(DPL_Type_Handles* handles, DPL_Types* types, Nob_String_View name)
{
    DPL_Type* type = dplt_find_by_name(types, name);
    if (!type) {
        return false;
    }

    dplt_add_handle(handles, type->handle);
    return true;
}

void dplt_print_meta_function(FILE* out, DPL_Types* types, DPL_Type* type)
{
    fprintf(out, "(");
    for (size_t i = 0; i < type->as.function.arguments.count; ++i) {
        if (i > 0) {
            fprintf(out, ", ");
        }
        dplt_print(out, types,
                   dplt_find_by_handle(types, type->as.function.arguments.items[i]));
    }
    fprintf(out, ")");

    fprintf(out, ": ");
    dplt_print(out, types, dplt_find_by_handle(types, type->as.function.returns));
}


void dplt_print(FILE* out, DPL_Types* types, DPL_Type* type)
{
    if (!type) {
        fprintf(out, "<unknown type>");
        return;
    }

    fprintf(out, SV_Fmt, SV_Arg(type->name));

    switch (type->kind) {
    case TYPE_FUNCTION:
        fprintf(out, "[");
        dplt_print_meta_function(out, types, type);
        fprintf(out, "]");
        break;
    default:
        break;
    }
}