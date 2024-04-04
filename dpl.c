#include "vm.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char** argv) {
    const char* exe = nob_shift_args(&argc, &argv);

    if (argc <= 0) {
        fprintf(stderr, "Usage: %s program.dplp\n", exe);
        exit(1);
    }

    const char* filename = nob_shift_args(&argc, &argv);

    DPL_Program program = {0};
    dplp_load(&program, filename);

    DPL_VirtualMachine vm = {0};
    dplv_init(&vm, &program);
    dplv_run(&vm);

    printf("VM stack size after completing execution: %zu\n", vm.stack_top);
    for (size_t i = 0; i < vm.stack_top; ++i)
    {
        printf("[ %f ]\n", vm.stack[i]);
    }

    dplv_free(&vm);
    dplp_free(&program);

    return 0;
}