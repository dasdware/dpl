#ifndef __DPL_TYPES_H
#define __DPL_TYPES_H

#include "nob.h"

typedef size_t DPL_Type_Handle;

typedef struct
{
    DPL_Type_Handle *items;
    size_t count;
    size_t capacity;
} DPL_Type_Handles;

typedef struct
{
} DPL_Type_AsBase;


typedef struct
{
    DPL_Type_Handles arguments;
    DPL_Type_Handle returns;
} DPL_Type_AsFunction;

typedef union {
    DPL_Type_AsBase base;
    DPL_Type_AsFunction function;
} DPL_Type_As;

typedef enum  {
    TYPE_BASE,
    TYPE_FUNCTION,
} DPL_Type_Kind;

typedef struct
{
    Nob_String_View name;
    DPL_Type_Handle handle;
    size_t hash;

    DPL_Type_Kind kind;
    DPL_Type_As as;
} DPL_Type;

typedef struct
{
    DPL_Type *items;
    size_t count;
    size_t capacity;

    // Common datatypes
    DPL_Type_Handle number_handle;

    // Common function types
    DPL_Type_Handle unary_handle;
    DPL_Type_Handle binary_handle;
} DPL_Types;

void dplt_init(DPL_Types *types);
void dplt_free(DPL_Types* types);

DPL_Type_Handle dplt_register_by_name(DPL_Types *types, Nob_String_View name);
DPL_Type_Handle dplt_register(DPL_Types* types, DPL_Type type);

DPL_Type* dplt_find_by_handle(DPL_Types *types, DPL_Type_Handle handle);
DPL_Type* dplt_find_by_name(DPL_Types *types, Nob_String_View name);

void dplt_add_handle(DPL_Type_Handles* handles, DPL_Type_Handle handle);
bool dplt_add_handle_by_name(DPL_Type_Handles* handles, DPL_Types* types, Nob_String_View name);

void dplt_print_meta_function(FILE* out, DPL_Types* types, DPL_Type* type);

void dplt_print(FILE* out, DPL_Types* types, DPL_Type* type);

#endif