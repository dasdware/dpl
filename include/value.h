#ifndef __DPL_VALUE_H
#define __DPL_VALUE_H

#include <nob.h>

typedef enum
{
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_BOOLEAN,
} DPL_ValueKind;

typedef struct
{
    DPL_ValueKind kind;
    union {
        double number;
        Nob_String_View string;
        bool boolean;
    } as;
} DPL_Value;

const char* dpl_value_kind_name(DPL_ValueKind kind);

DPL_Value dpl_value_make_number(double value);
int dpl_value_compare_numbers(double a, double b);
const char *dpl_value_format_number(double value);

DPL_Value dpl_value_make_string(Nob_String_View value);

DPL_Value dpl_value_make_boolean(bool value);
const char *dpl_value_format_boolean(bool value);

void dpl_value_print_number(double value);
void dpl_value_print_string(Nob_String_View value);
void dpl_value_print_boolean(bool value);
void dpl_value_print(DPL_Value value);

bool dpl_value_number_equals(double number1, double number2);
bool dpl_value_string_equals(Nob_String_View string1, Nob_String_View string2);
bool dpl_value_equals(DPL_Value value1, DPL_Value value2);

#endif // __DPL_VALUE_H