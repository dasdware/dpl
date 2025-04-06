#ifndef __DPL_INTRINSICS_H
#define __DPL_INTRINSICS_H

typedef enum
{
    INTRINSIC_NUMBER_ITERATOR,
    INTRINSIC_NUMBER_ITERATOR_NEXT,

    COUNT_INTRINSICS,
} DPL_Intrinsic_Kind;

const char *dpl_intrinsic_kind_name(DPL_Intrinsic_Kind kind);

#endif // __DPL_INTRINSICS_H