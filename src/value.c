#ifdef DPL_LEAKCHECK
#   include "stb_leakcheck.h"
#endif

#include <math.h>

#include "error.h"
#include "value.h"

const char* dpl_value_kind_name(DPL_ValueKind kind) {
    switch (kind) {
    case VALUE_NUMBER:
        return "number";
    case VALUE_STRING:
        return "string";
    case VALUE_BOOLEAN:
        return "boolean";
    }

    DW_ERROR("ERROR: Invalid value kind `%02X`.", kind);
}

DPL_Value dpl_value_make_number(double value) {
    return (DPL_Value) {
        .kind = VALUE_NUMBER,
        .as = {
            .number = value
        }
    };
}

DPL_Value dpl_value_make_string(Nob_String_View value) {
    return (DPL_Value) {
        .kind = VALUE_STRING,
        .as = {
            .string = value
        }
    };
}

DPL_Value dpl_value_make_boolean(bool value) {
    return (DPL_Value) {
        .kind = VALUE_BOOLEAN,
        .as = {
            .boolean = value
        }
    };
}

#define EPSILON 0.00001

int dpl_value_compare_numbers(double a, double b) {
    if (fabs(a - b) < EPSILON) {
        return 0;
    }
    if (a < b) {
        return -1;
    }
    return 1;
}

const char* dpl_value_format_number(double value) {
    static char buffer[16];
    double abs_value = fabs(value);
    if (abs_value - floorf(abs_value) < EPSILON) {
        snprintf(buffer, 16, "%i", (int) round(value));
    } else {
        snprintf(buffer, 16, "%f", value);
    }
    return buffer;
}

void dpl_value_print_number(double value) {
    printf("[%s: %s]", dpl_value_kind_name(VALUE_NUMBER), dpl_value_format_number(value));
}

void dpl_value_print_string(Nob_String_View value) {
    printf("[%s: \"",  dpl_value_kind_name(VALUE_STRING));

    const char* pos = value.data;
    for (size_t i = 0; i < value.count; ++i) {
        switch (*pos) {
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

const char* dpl_value_format_boolean(bool value) {
    return value ? "true" : "false";
}

void dpl_value_print_boolean(bool value) {
    printf("[%s: %s]", dpl_value_kind_name(VALUE_BOOLEAN), dpl_value_format_boolean(value));
}

void dpl_value_print(DPL_Value value) {
    switch (value.kind) {
    case VALUE_NUMBER:
        dpl_value_print_number(value.as.number);
        break;
    case VALUE_STRING:
        dpl_value_print_string(value.as.string);
        break;
    case VALUE_BOOLEAN:
        dpl_value_print_boolean(value.as.boolean);
        break;
    default:
        DW_ERROR("Cannot debug print value of kind `%s`.",
                 dpl_value_kind_name(value.kind));
    }
}

bool dpl_value_number_equals(double number1, double number2)
{
    return fabs(number1 - number2) < EPSILON;
}

bool dpl_value_string_equals(Nob_String_View string1, Nob_String_View string2)
{
    return nob_sv_eq(string1, string2);
}


bool dpl_value_equals(DPL_Value value1, DPL_Value value2) {
    if (value1.kind != value2.kind) {
        return false;
    }

    switch (value1.kind) {
    case VALUE_NUMBER:
        return dpl_value_number_equals(value1.as.number, value2.as.number);
    case VALUE_STRING:
        return dpl_value_string_equals(value1.as.string, value2.as.string);
    default:
        DW_ERROR("Cannot compare values of unknown kind `%d`.", value1.kind);
    }
}
