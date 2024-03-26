#ifndef __DPL_TYPES_H
#define __DPL_TYPES_H

#include "nob.h"


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