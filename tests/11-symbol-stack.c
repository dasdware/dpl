#include <stdio.h>
#include <dpl/symbols.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define NOB_IMPLEMENTATION
#include "nob.h"
#undef NOB_IMPLEMENTATION

#define DW_MEMORY_TABLE_IMPLEMENTATION
#include <dw_memory_table.h>

#define DW_BYTEBUFFER_IMPLEMENTATION
#include <dw_byte_buffer.h>

#define DW_ARRAY_IMPLEMENTATION
#include <dw_array.h>

#include "externals.h"

#include <error.h>

void test_find_symbol(DPL_SymbolStack *stack, const char *name)
{
    DPL_Symbol *symbol = dpl_symbols_find_cstr(stack, name);
    if (!symbol)
    {
        printf("%s\n", dpl_symbols_last_error());
    }
    else
    {
        Nob_String_Builder sb = {0};
        dpl_symbols_print_full_signature(symbol, &sb);
        nob_sb_append_null(&sb);

        printf("Found %s successfully.\n", sb.items);

        nob_sb_free(sb);
    }
}

int main()
{
    DPL_SymbolStack stack = {0};
    dpl_symbols_init(&stack);

    dpl_symbols_push_boundary_cstr(&stack, NULL, BOUNDARY_SCOPE);

    dpl_symbols_push_constant_number_cstr(&stack, "PI", 3.14159);
    dpl_symbols_push_var_cstr(&stack, "x", "String");

    DPL_ExternalFunctions externals = NULL;
    dple_init(&externals);

    for (size_t i = 0; i < da_size(externals); ++i)
    {
        DPL_Symbol *function = dpl_symbols_push_function_external_cstr(
            &stack,
            externals[i].name, externals[i].return_type,
            da_size(externals[i].argument_types), externals[i].argument_types,
            i);

        if (!function)
        {
            Nob_String_Builder sb = {0};
            nob_sb_append_cstr(&sb, "Cannot add external function `");
            nob_sb_append_cstr(&sb, externals[i].name);
            nob_sb_append_cstr(&sb, "(");

            for (size_t j = 0; j < da_size(externals[i].argument_types); ++j)
            {
                if (j > 0)
                {
                    nob_sb_append_cstr(&sb, ", ");
                }
                nob_sb_append_cstr(&sb, externals[i].argument_types[j]);
            }
            nob_sb_append_cstr(&sb, "): ");
            nob_sb_append_cstr(&sb, externals[i].return_type);
            nob_sb_append_cstr(&sb, "`: ");
            nob_sb_append_cstr(&sb, dpl_symbols_last_error());

            nob_sb_append_null(&sb);

            DW_ERROR("%s", sb.items);
        }
    }

    dpl_symbols_push_boundary_cstr(&stack, NULL, BOUNDARY_FUNCTION);
    dpl_symbols_push_argument_cstr(&stack, "foo", "Boolean");
    dpl_symbols_push_argument_cstr(&stack, "bar", "Boolean");
    dpl_symbols_push_var_cstr(&stack, "baz", "Boolean");

    test_find_symbol(&stack, "PI");
    test_find_symbol(&stack, "bar");

    test_find_symbol(&stack, "x");
    dpl_symbols_pop_boundary(&stack);
    test_find_symbol(&stack, "x");

    dpl_symbols_print_table(&stack);

    dpl_symbols_free(&stack);
}