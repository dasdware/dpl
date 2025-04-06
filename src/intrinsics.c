#include <assert.h>

#include <error.h>

#include <dpl/intrinsics.h>

const char *INTRINSIC_KIND_NAMES[COUNT_INTRINSICS] = {
    [INTRINSIC_NUMBER_ITERATOR] = "iterator(Range<Number>): Iterator<Number>",
    [INTRINSIC_NUMBER_ITERATOR_NEXT] = "next(Iterator<Number>): Iterator<Number>",
};

static_assert(COUNT_INTRINSICS == 2,
              "Count of intrinsic kinds has changed, please update intrinsic kind names map.");

const char *dpl_intrinsic_kind_name(DPL_Intrinsic_Kind kind)
{
    if (kind >= COUNT_INTRINSICS)
    {
        DW_ERROR("Invalid intrinsic kind %u.", kind);
    }
    return INTRINSIC_KIND_NAMES[kind];
}