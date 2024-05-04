#include <math.h>

#include "error.h"
#include "value.h"

const char* dpl_value_kind_name(DPL_ValueKind kind) {
    switch (kind) {
    case VALUE_NUMBER:
        return "number";
    case VALUE_STRING:
        return "string";
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

#define EPSILON 0.00001

const char* dpl_value_format_number(DPL_Value value) {
    static char buffer[16];
    double abs_value = fabs(value.as.number);
    if (abs_value - floorf(abs_value) < EPSILON) {
        snprintf(buffer, 16, "%i", (int) round(value.as.number));
    } else {
        snprintf(buffer, 16, "%f", value.as.number);
    }
    return buffer;
}

void dpl_value_print(DPL_Value value) {
    const char* kind_name = dpl_value_kind_name(value.kind);
    switch (value.kind) {
    case VALUE_NUMBER:
        printf("[%s: %s]", kind_name, dpl_value_format_number(value));
        break;
    case VALUE_STRING: {
        printf("[%s: \"", kind_name);

        const char* pos = value.as.string.data;
        for (size_t i = 0; i < value.as.string.count; ++i) {
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
    break;
    default:
        DW_ERROR("Cannot debug print value of kind `%s`.", kind_name);
    }
}

