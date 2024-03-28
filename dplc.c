#include "dpl.h"
#include "vm.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char** argv) {
    const char* program = nob_shift_args(&argc, &argv);

    if (argc <= 0) {
        fprintf(stderr, "Usage: %s sourceFileName\n", program);
        exit(1);
    }

    const char* source_filename = nob_shift_args(&argc, &argv);

    Nob_String_Builder source = {0};
    nob_read_entire_file(source_filename, &source);

    DPL dpl = {0};
    dpl.debug = true;
    dpl_init(&dpl, source_filename, nob_sv_from_parts(source.items, source.count));

    DPL_Program compiled_program = {0};
    dplp_init(&compiled_program);

    dpl_compile(&dpl, &compiled_program);

    DPL_VirtualMachine vm = {0};
    dplv_init(&vm, &compiled_program);

    dplv_run(&vm);

    printf("VM stack size after completing execution: %zu\n", vm.stack_top);

    for (size_t i = 0; i < vm.stack_top; ++i)
    {
        printf("[ %f ]\n", vm.stack[i]);
    }

    dplv_free(&vm);
    dplp_free(&compiled_program);
    dpl_free(&dpl);
    nob_da_free(source);

    return 0;
}