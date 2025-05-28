#include <assert.h>

#include <error.h>

#include <dpl/intrinsics.h>

const char *INTRINSIC_KIND_NAMES[COUNT_INTRINSICS] = {
    [INTRINSIC_BOOLEAN_TOSTRING] = "toString(Boolean): String",
    [INTRINSIC_BOOLEAN_PRINT] = "print(Boolean): Boolean",
    [INTRINSIC_NUMBER_PRINT] = "print(Number): Number",
    [INTRINSIC_NUMBER_TOSTRING] = "toString(Number): String",
    [INTRINSIC_NUMBERITERATOR_NEXT] = "next(Iterator<Number>): Iterator<Number>",
    [INTRINSIC_NUMBERRANGE_ITERATOR] = "iterator(Range<Number>): Iterator<Number>",
    [INTRINSIC_STRING_LENGTH] = "length(String): Number",
    [INTRINSIC_STRING_PRINT] = "print(String): String",
    [INTRINSIC_ARRAY_LENGTH] = "<T>length([T]): Number",
    [INTRINSIC_ARRAY_ELEMENT] = "element([T], Number): T",
    [INTRINSIC_ARRAY_ITERATOR] = "<T>iterator([T]): Iterator<T>",
    [INTRINSIC_ARRAYITERATOR_NEXT] = "next(Iterator<T>): Iterator<T>",
};

static_assert(COUNT_INTRINSICS == 12,
              "Count of intrinsic kinds has changed, please update intrinsic kind names map.");

const char *dpl_intrinsic_kind_name(DPL_Intrinsic_Kind kind)
{
    if (kind >= COUNT_INTRINSICS)
    {
        DW_ERROR("Invalid intrinsic kind %u.", kind);
    }
    return INTRINSIC_KIND_NAMES[kind];
}