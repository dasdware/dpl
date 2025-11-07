#ifdef DPL_LEAKCHECK
#include "stb_leakcheck.h"
#endif

#include <math.h>

#include "error.h"
#include "value.h"

const char *dpl_value_kind_name(DPL_ValueKind kind)
{
    switch (kind)
    {
    case VALUE_NUMBER:
        return "number";
    case VALUE_STRING:
        return "string";
    case VALUE_BOOLEAN:
        return "boolean";
    case VALUE_OBJECT:
        return "object";
    case VALUE_ARRAY:
        return "array";
    }

    DW_UNIMPLEMENTED_MSG("ERROR: Invalid value kind `%02X`.", kind);
}

DPL_Value dpl_value_make_number(double value)
{
    return (DPL_Value){
        .kind = VALUE_NUMBER,
        .as = {
            .number = value}};
}

DPL_Value dpl_value_make_string(Nob_String_View value)
{
    return (DPL_Value){
        .kind = VALUE_STRING,
        .as = {
            .string = value}};
}

DPL_Value dpl_value_make_boolean(bool value)
{
    return (DPL_Value){
        .kind = VALUE_BOOLEAN,
        .as = {
            .boolean = value}};
}

DPL_Value dpl_value_make_object(DW_MemoryTable_Item *value)
{
    return (DPL_Value){
        .kind = VALUE_OBJECT,
        .as = {
            .object = value}};
}

DPL_Value dpl_value_make_array_slot()
{
    return (DPL_Value){
        .kind = VALUE_ARRAY,
        .as = {
            .array = NULL}};
}

int dpl_value_compare_numbers(double a, double b)
{
    if (fabs(a - b) < DPL_VALUE_EPSILON)
    {
        return 0;
    }
    if (a < b)
    {
        return -1;
    }
    return 1;
}

const char *dpl_value_format_number(double value)
{
    static char buffer[16];
    double abs_value = fabs(value);
    if (abs_value - floorf(abs_value) < DPL_VALUE_EPSILON)
    {
        snprintf(buffer, 16, "%i", (int)round(value));
    }
    else
    {
        snprintf(buffer, 16, "%f", value);
    }
    return buffer;
}

void dpl_value_print_number(double value)
{
    printf("[%s: %s]", dpl_value_kind_name(VALUE_NUMBER), dpl_value_format_number(value));
}

void dpl_value_print_string(Nob_String_View value)
{
    printf("[%s: \"", dpl_value_kind_name(VALUE_STRING));

    const char *pos = value.data;
    for (size_t i = 0; i < value.count; ++i)
    {
        switch (*pos)
        {
        case '\n':
            printf("\\n");
            break;
        case '\r':
            printf("\\r");
            break;
        case '\t':
            printf("\\t");
            break;
        default:
            printf("%c", *pos);
        }
        ++pos;
    }
    printf("\"]");
}

const char *dpl_value_format_boolean(bool value)
{
    return value ? "true" : "false";
}

void dpl_value_print_boolean(bool value)
{
    printf("[%s: %s]", dpl_value_kind_name(VALUE_BOOLEAN), dpl_value_format_boolean(value));
}

uint8_t dpl_value_object_field_count(DW_MemoryTable_Item *object)
{
    return object->length / sizeof(DPL_Value);
}

DPL_Value dpl_value_object_get_field(DW_MemoryTable_Item *object, uint8_t field_index)
{
    return ((DPL_Value *)object->data)[field_index];
}

void dpl_value_print_object(DW_MemoryTable_Item *object)
{
    uint8_t field_count = dpl_value_object_field_count(object);
    printf("[%s(%d): ", dpl_value_kind_name(VALUE_OBJECT), field_count);
    for (uint8_t field_index = 0; field_index < field_count; ++field_index)
    {
        dpl_value_print(dpl_value_object_get_field(object, field_index));
    }
    printf("]");
}

uint8_t dpl_value_array_element_count(DW_MemoryTable_Item *array)
{
    return array->length / sizeof(DPL_Value);
}

DPL_Value dpl_value_array_get_element(DW_MemoryTable_Item *array, uint8_t element_index)
{
    return ((DPL_Value *)array->data)[element_index];
}

void dpl_value_print_array(DW_MemoryTable_Item *array)
{
    if (array == NULL)
    {
        printf("[%s slot]", dpl_value_kind_name(VALUE_ARRAY));
        return;
    }

    uint8_t element_count = dpl_value_array_element_count(array);
    printf("[%s(%d): ", dpl_value_kind_name(VALUE_ARRAY), element_count);
    for (uint8_t element_index = 0; element_index < element_count; ++element_index)
    {
        dpl_value_print(dpl_value_array_get_element(array, element_index));
    }
    printf("]");
}

void dpl_value_print(DPL_Value value)
{
    switch (value.kind)
    {
    case VALUE_NUMBER:
        dpl_value_print_number(value.as.number);
        break;
    case VALUE_STRING:
        dpl_value_print_string(value.as.string);
        break;
    case VALUE_BOOLEAN:
        dpl_value_print_boolean(value.as.boolean);
        break;
    case VALUE_OBJECT:
        dpl_value_print_object(value.as.object);
        break;
    case VALUE_ARRAY:
        dpl_value_print_array(value.as.array);
        break;
    default:
        DW_UNIMPLEMENTED_MSG("Cannot debug print value of kind `%s`.",
                             dpl_value_kind_name(value.kind));
    }
}

bool dpl_value_number_equals(double number1, double number2)
{
    return fabs(number1 - number2) < DPL_VALUE_EPSILON;
}

bool dpl_value_string_equals(Nob_String_View string1, Nob_String_View string2)
{
    return nob_sv_eq(string1, string2);
}

bool dpl_value_equals(DPL_Value value1, DPL_Value value2)
{
    if (value1.kind != value2.kind)
    {
        return false;
    }

    switch (value1.kind)
    {
    case VALUE_NUMBER:
        return dpl_value_number_equals(value1.as.number, value2.as.number);
    case VALUE_STRING:
        return dpl_value_string_equals(value1.as.string, value2.as.string);
    default:
        DW_ERROR("Cannot compare values of unknown kind `%d`.", value1.kind);
    }
}
