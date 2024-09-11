#ifndef __DPL_EXTERNALS_H
#define __DPL_EXTERNALS_H

#include "dw_array.h"

typedef const char *DPL_External_TypeName;

typedef da_array(DPL_External_TypeName) DPL_External_TypeNames;

struct DPL_VirtualMachine;
typedef void (*DPL_ExternalFunction_Callback)(struct DPL_VirtualMachine *vm);

typedef struct
{
    const char *name;
    DPL_External_TypeNames argument_types;
    DPL_External_TypeName return_type;
    DPL_ExternalFunction_Callback callback;

} DPL_ExternalFunction;

typedef da_array(DPL_ExternalFunction) DPL_ExternalFunctions;

void dple_init(DPL_ExternalFunctions *externals);
void dple_free(DPL_ExternalFunctions *externals);

DPL_ExternalFunction *dple_add_by_name(DPL_ExternalFunctions *externals, const char *name);

#endif // __DPL_EXTERNALS_H