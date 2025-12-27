// DEFINE: DPL_MEMORYVALUE_POOL_IDS
// SOURCE: ./src/value.c
#include <stdio.h>

#define STB_LEAKCHECK_IMPLEMENTATION
#include <stb_leakcheck.h>

#define ARENA_IMPLEMENTATION
#define NOB_IMPLEMENTATION
#include <dpl/value.h>

int main()
{
    DPL_MemoryValue_Pool pool = {0};

    printf("Initialization\n");

    const char* hello_world = "Hello, World!\n";
    const DPL_Value string = dpl_value_make_string(&pool, strlen(hello_world), hello_world);
    const DPL_Value object = dpl_value_make_object(&pool, DPL_VALUES(dpl_value_make_boolean(true), dpl_value_make_number(123)));
    const DPL_Value array = dpl_value_make_array(&pool, DPL_VALUES(dpl_value_make_number(1), dpl_value_make_number(2), dpl_value_make_number(3), dpl_value_make_number(4), dpl_value_make_number(5)));
    dpl_value_pool_print(&pool);

    printf("Acquiring\n");
    dpl_value_pool_acquire_item(&pool, string.as.string);
    dpl_value_pool_acquire_item(&pool, object.as.object);
    dpl_value_pool_print(&pool);

    printf("Releasing array\n");
    dpl_value_pool_release_item(&pool, array.as.array);
    dpl_value_pool_print(&pool);

    printf("Re-allocating another string\n");
    const DPL_Value string2 = dpl_value_make_string(&pool, strlen(hello_world), hello_world);
    dpl_value_pool_print(&pool);

    printf("Releasing everything\n");
    dpl_value_pool_release_item(&pool, string.as.string);
    dpl_value_pool_release_item(&pool, string.as.string);
    dpl_value_pool_release_item(&pool, object.as.object);
    dpl_value_pool_release_item(&pool, object.as.object);
    dpl_value_pool_release_item(&pool, string2.as.string);
    dpl_value_pool_print(&pool);

    printf("Freeing pool\n");
    dpl_value_pool_free(&pool);
    dpl_value_pool_print(&pool);

    stb_leakcheck_dumpmem();
    return 0;
}
