#include "dpl.h"
#include "error.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define DW_ARRAY_IMPLEMENTATION
#include <dw_array.h>

#define DW_STRING_TABLE_IMPLEMENTATION
#include <string_table.h>

#define DW_BYTEBUFFER_IMPLEMENTATION
#include <dw_byte_buffer.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

void usage(const char* program)
{
    DW_ERROR("Usage: %s [-d] [-o output_file] source.dpl", program);
}

int main(int argc, char** argv) {
    const char* program = nob_shift_args(&argc, &argv);

    DPL_ExternalFunctions externals = {0};
    dple_init(&externals);

    DPL dpl = {0};

    const char* source_filename = NULL;
    const char* output_filename = NULL;
    while (argc > 0)
    {
        char* arg = nob_shift_args(&argc, &argv);
        if (strcmp(arg, "-d") == 0) {
            dpl.debug = true;
        } else if (strcmp(arg, "-o") == 0) {
            if (argc == 0) {
                DW_ERROR_MSGLN("Option -o expects an output filename.");
                usage(program);
            }
            output_filename = nob_shift_args(&argc, &argv);
        } else {
            source_filename = arg;
        }
    }

    if (source_filename == NULL) {
        DW_ERROR_MSGLN("ERROR: No source file given.");
        usage(program);
    }
    if (output_filename == NULL) {
        output_filename = nob_temp_change_file_ext(source_filename, "dplp");
    }

    Nob_String_Builder source = {0};
    nob_read_entire_file(source_filename, &source);

    dpl.file_name = nob_sv_from_cstr(source_filename);
    dpl.source = nob_sv_from_parts(source.items, source.count);
    dpl_init(&dpl, externals);

    DPL_Program compiled_program = {0};
    dplp_init(&compiled_program);
    dpl_compile(&dpl, &compiled_program);
    dplp_save(&compiled_program, output_filename);

    dplp_free(&compiled_program);
    dpl_free(&dpl);
    dple_free(&externals);
    nob_da_free(source);

    return 0;
}