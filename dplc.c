#include "dpl.h"

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

    DPL_ByteCode bytecode = {0};
    dplb_init(&bytecode);

    dpl_compile(&dpl, &bytecode);

    dplb_free(&bytecode);
    dpl_free(&dpl);
    nob_da_free(source);

    return 0;
}