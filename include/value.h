#ifndef __DPL_VALUE_H
#define __DPL_VALUE_H

#include <nob.h>

typedef enum
{
    VALUE_NUMBER,
    VALUE_STRING,
} DPL_ValueKind;

typedef union
{
    double number;
    Nob_String_View string;
} DPL_Value_As;

typedef struct
{
    DPL_ValueKind kind;
    DPL_Value_As as;
} DPL_Value;

const char* dpl_value_kind_name(DPL_ValueKind kind);

DPL_Value dpl_value_make_number(double value);
const char *dpl_value_format_number(DPL_Value value);

DPL_Value dpl_value_make_string(Nob_String_View value);

void dpl_value_print(DPL_Value value);

#endif // __DPL_VALUE_H