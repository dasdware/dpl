#include "error.h"
#include "externals.h"
#include "value.h"
#include "vm.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

#define DW_STRING_TABLE_IMPLEMENTATION
#include <string_table.h>

int main(int argc, char** argv) {
    const char* exe = nob_shift_args(&argc, &argv);

    if (argc <= 0) {
        DW_ERROR("Usage: %s program.dplp", exe);
    }

    const char* filename = nob_shift_args(&argc, &argv);

    DPL_ExternalFunctions externals = {0};
    dple_init(&externals);

    DPL_Program program = {0};
    dplp_load(&program, filename);

    DPL_VirtualMachine vm = {0};
    dplv_init(&vm, &program, &externals);
    dplv_run(&vm);

    if (vm.debug) {
        printf("==[ DEBUG ]=============================\n");
        printf("VM stack after execution:\n");
        for (size_t i = 0; i < vm.stack_top; ++i) {
            printf("* %zu: ", i);
            dpl_value_print(vm.stack[i]);
            printf("\n");
        }
        printf("Entries remaining in string table: %zu\n", ST_CAPACITY - vm.strings.free_items.count);
        printf("==[ /DEBUG ]============================\n");
    }

    dplv_free(&vm);
    dplp_free(&program);

    return 0;
}