#ifndef __DPL_GENERATOR_H
#define __DPL_GENERATOR_H

#include <dpl/binding.h>
#include <dpl/program.h>

typedef struct
{
    DPL_Binding_UserFunctions user_functions;
} DPL_Generator;

void dpl_generate(DPL_Generator *generator, DPL_Bound_Node *node, DPL_Program *program);

#endif // __DPL_GENERATOR_H