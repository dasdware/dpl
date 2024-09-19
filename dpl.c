#ifdef DPL_LEAKCHECK
#   define STB_LEAKCHECK_IMPLEMENTATION
#   include "stb_leakcheck.h"
#endif

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

#define DW_BYTEBUFFER_IMPLEMENTATION
#include <dw_byte_buffer.h>

#define DW_ARRAY_IMPLEMENTATION
#include <dw_array.h>

void usage(const char* program)
{
    DW_ERROR("Usage: %s [-d] [-t] program.dplp", program);
}

int main(int argc, char** argv) {
    const char* exe = nob_shift_args(&argc, &argv);

    DPL_VirtualMachine vm = {0};

    char* program_filename = NULL;
    while (argc > 0)
    {
        char* arg = nob_shift_args(&argc, &argv);
        if (strcmp(arg, "-d") == 0) {
            vm.debug = true;
        } else if (strcmp(arg, "-t") == 0) {
            vm.trace = true;
        } else {
            program_filename = arg;
        }
    }

    if (program_filename == NULL) {
        DW_ERROR_MSGLN("ERROR: No program file given.");
        usage(exe);
    }

    DPL_ExternalFunctions externals = {0};
    dple_init(&externals);

    DPL_Program program = {0};
    dplp_load(&program, program_filename);

    dplv_init(&vm, &program, externals);
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

#ifdef DPL_LEAKCHECK
    stb_leakcheck_dumpmem();
#endif

    return 0;
}