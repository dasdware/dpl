#ifdef DPL_LEAKCHECK
#define STB_LEAKCHECK_IMPLEMENTATION
#include <stb_leakcheck.h>
#endif

#include <dpl.h>
#include <dw_error.h>

#define ARENA_IMPLEMENTATION
#include <arena.h>

#define DW_MEMORY_TABLE_IMPLEMENTATION
#include <dw_memory_table.h>

#define DW_BYTEBUFFER_IMPLEMENTATION
#include <dw_byte_buffer.h>

#define NOB_IMPLEMENTATION
#include <nob.h>
#include <nobx.h>

void usage(const char *program)
{
    DW_ERROR("Usage: %s [-d] [-o output_file] source.dpl", program);
}

int main(int argc, char **argv)
{
    const char *program = nob_shift_args(&argc, &argv);

    Arena memory = {0};

    DPL dpl = {0};
    dpl.memory = &memory;

    const char *source_filename = NULL;
    const char *output_filename = NULL;
    while (argc > 0)
    {
        char *arg = nob_shift_args(&argc, &argv);
        if (strcmp(arg, "-d") == 0)
        {
            dpl.debug = true;
        }
        else if (strcmp(arg, "-o") == 0)
        {
            if (argc == 0)
            {
                DW_ERROR_MSGLN("Option -o expects an output filename.");
                usage(program);
            }
            output_filename = nob_shift_args(&argc, &argv);
        }
        else
        {
            source_filename = arg;
        }
    }

    if (source_filename == NULL)
    {
        DW_ERROR_MSGLN("ERROR: No source file given.");
        usage(program);
    }

    Nob_String_Builder output_filename_sb = {0};
    if (output_filename == NULL)
    {
        char *ext = strrchr(source_filename, '.');
        if (ext)
        {
            nob_sb_append_buf(&output_filename_sb, source_filename, ext - source_filename);
        }
        else
        {
            nob_sb_append_cstr(&output_filename_sb, source_filename);
            nob_sb_append_cstr(&output_filename_sb, ".");
        }
        nob_sb_append_cstr(&output_filename_sb, "dplp");
        nob_sb_append_null(&output_filename_sb);
    }
    else
    {
        nob_sb_append_cstr(&output_filename_sb, output_filename);
        nob_sb_append_null(&output_filename_sb);
    }

    dpl.file_name = nob_sv_from_cstr(source_filename);

    Nob_String_Builder source = {0};
    nob_read_entire_file(source_filename, &source);
    dpl.source = nob_sv_from_parts(source.items, source.count);

    dpl_init(&dpl);

    DPL_Program compiled_program = {0};
    dplp_init(&compiled_program);
    dpl_compile(&dpl, &compiled_program);
    dplp_save(&compiled_program, output_filename_sb.items);
    nob_sb_free(output_filename_sb);

    dplp_free(&compiled_program);
    dpl_free(&dpl);
    nob_da_free(source);

    arena_free(&memory);

#ifdef DPL_LEAKCHECK
    stb_leakcheck_dumpmem();
#endif

    return 0;
}