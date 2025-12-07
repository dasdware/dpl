#ifdef DPL_LEAKCHECK
#define STB_LEAKCHECK_IMPLEMENTATION
#include <stb_leakcheck.h>
#endif

#include <dw_error.h>
#include <dpl/value.h>
#include <dpl/vm/vm.h>

#define ARENA_IMPLEMENTATION
#include <arena.h>

#define NOB_IMPLEMENTATION
#include <nob.h>
#include <nobx.h>

#define DW_MEMORY_TABLE_IMPLEMENTATION
#include <dw_memory_table.h>

#define DW_BYTEBUFFER_IMPLEMENTATION
#include <dw_byte_buffer.h>

void usage(const char *program)
{
    DW_ERROR("Usage: %s [-d] [-t] program.dplp", program);
}

int main(int argc, char **argv)
{
    const char *exe = nob_shift_args(&argc, &argv);

    DPL_VirtualMachine vm = {0};

    char *program_filename = NULL;
    while (argc > 0)
    {
        char *arg = nob_shift_args(&argc, &argv);
        if (strcmp(arg, "-d") == 0)
        {
            vm.debug = true;
        }
        else if (strcmp(arg, "-t") == 0)
        {
            vm.trace = true;
        }
        else
        {
            program_filename = arg;
        }
    }

    if (program_filename == NULL)
    {
        DW_ERROR_MSGLN("ERROR: No program file given.");
        usage(exe);
    }

    DPL_Program program = {0};
    dplp_load(&program, program_filename);

    dplv_init(&vm, &program);
    dplv_run(&vm);

    if (vm.debug)
    {
        printf("\n================================================================\n");
        printf("| VM stack after execution\n");
        printf("================================================================\n");
        for (size_t i = 0; i < vm.stack_top; ++i)
        {
            printf("| #%02zu: ", i);
            dpl_value_print(vm.stack[i]);
            printf("\n");
        }
        printf("================================================================\n\n");
        mt_print(&vm.stack_memory);
    }

    dplv_free(&vm);
    dplp_free(&program);

#ifdef DPL_LEAKCHECK
    stb_leakcheck_dumpmem();
#endif

    return 0;
}