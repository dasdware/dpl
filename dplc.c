#include "dpl.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

void usage(const char* program)
{
    fprintf(stderr, "Usage: %s [-d] source.dpl\n", program);
}

int main(int argc, char** argv) {
    const char* program = nob_shift_args(&argc, &argv);

    DPL dpl = {0};
    dpl.debug = true;

    char* source_filename = NULL;
    while (argc > 0)
    {
        char* arg = nob_shift_args(&argc, &argv);
        if (strcmp(arg, "-d") == 0) {
            dpl.debug = true;
        } else {
            source_filename = arg;
        }
    }

    if (source_filename == NULL) {
        fprintf(stderr, "ERROR: No source file given.\n");
        usage(program);
        exit(1);
    }

    Nob_String_Builder source = {0};
    nob_read_entire_file(source_filename, &source);

    dpl.file_name = nob_sv_from_cstr(source_filename);
    dpl.source = nob_sv_from_parts(source.items, source.count);
    dpl_init(&dpl);

    DPL_Program compiled_program = {0};
    dplp_init(&compiled_program);
    dpl_compile(&dpl, &compiled_program);
    dplp_save(&compiled_program, nob_temp_change_file_ext(source_filename, "dplp"));

    dplp_free(&compiled_program);
    dpl_free(&dpl);
    nob_da_free(source);

    return 0;
}