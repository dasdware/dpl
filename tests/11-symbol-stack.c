// SOURCE: ./src/intrinsics.c
// SOURCE: ./src/program.c
// SOURCE: ./src/value.c
// SOURCE: ./src/symbols.c

#include <stdio.h>
#include <dpl/symbols.h>

#define ARENA_IMPLEMENTATION
#include <arena.h>

#define NOB_IMPLEMENTATION
#include <nob.h>
#include <nobx.h>
#undef NOB_IMPLEMENTATION

#define DW_MEMORY_TABLE_IMPLEMENTATION
#include <dw_memory_table.h>

#define DW_BYTEBUFFER_IMPLEMENTATION
#include <dw_byte_buffer.h>

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

    dpl_symbols_push_type_base_cstr(&stack, TYPENAME_NUMBER, TYPE_BASE_NUMBER);
    dpl_symbols_push_type_base_cstr(&stack, TYPENAME_STRING, TYPE_BASE_STRING);
    dpl_symbols_push_type_base_cstr(&stack, TYPENAME_BOOLEAN, TYPE_BASE_BOOLEAN);

    dpl_symbols_push_boundary_cstr(&stack, NULL, BOUNDARY_SCOPE);

    dpl_symbols_push_constant_number_cstr(&stack, "PI", 3.14159);
    dpl_symbols_push_var_cstr(&stack, "x", "String");

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