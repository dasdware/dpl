#ifndef __DPL_FUNCTIONS_H
#define __DPL_FUNCTIONS_H

#include "nob.h"
#include "types.h"

typedef size_t DPL_Function_Handle;

typedef struct
{
    DPL_Function_Handle handle;
    Nob_String_View name;
    DPL_Type_Handle type;
} DPL_Function;

typedef struct
{
    DPL_Function *items;
    size_t count;
    size_t capacity;
} DPL_Functions;

void dplf_init(DPL_Functions* functions, DPL_Types* types);
void dplf_free(DPL_Functions* functions);

DPL_Function_Handle dplf_register(DPL_Functions* functions, Nob_String_View name,

                                  DPL_Type_Handle type);
DPL_Function* dplf_find_by_handle(DPL_Functions *functions, DPL_Function_Handle handle);
DPL_Function* dplf_find_by_signature1(DPL_Types* types, DPL_Functions *functions,
                                      Nob_String_View name, DPL_Type_Handle arg0);
DPL_Function* dplf_find_by_signature2(DPL_Types* types, DPL_Functions *functions,
                                      Nob_String_View name, DPL_Type_Handle arg0, DPL_Type_Handle arg1);


void dplf_print(FILE* out, DPL_Types* types, DPL_Function* function);

#endif // __DPL_FUNCTIONS_H