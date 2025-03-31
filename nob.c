#include "./include/error.h"
#define NOB_IMPLEMENTATION
#include "./include/nob.h"

#define BUILD_DIR "build"
#define BUILD_OUTPUT(output) "." NOB_PATH_DELIM_STR BUILD_DIR NOB_PATH_DELIM_STR output

#define DPLC_OUTPUT BUILD_OUTPUT("dplc.exe")
#define DPL_OUTPUT BUILD_OUTPUT("dpl.exe")

#define COMMAND_DELIM nob_sv_from_cstr("--")

#define COMMAND_BUILD nob_sv_from_cstr("build")
#define COMMAND_CMD nob_sv_from_cstr("cmd")
#define COMMAND_HELP nob_sv_from_cstr("help")
#define COMMAND_RUN nob_sv_from_cstr("run")
#define COMMAND_TEST nob_sv_from_cstr("test")

#define TARGET_DPLC nob_sv_from_cstr("dplc")
#define TARGET_DPL nob_sv_from_cstr("dpl")

void usage(Nob_String_View program, bool help_hint)
{
    printf(
        "\n"
        "Usage:\n" SV_Fmt " command [parameters] [-- command [parameters]]*\n",
        SV_Arg(program));

    if (help_hint)
    {
        printf(
            "\n"
            "Use the following command for further details:\n"
            " " SV_Fmt " help\n",
            SV_Arg(program));
    }
}

void check_command_end(Nob_String_View program, int *argc, char ***argv, const char *command)
{
    if (*argc > 0 && !nob_sv_eq(nob_sv_shift_args(argc, argv), COMMAND_DELIM))
    {
        nob_log(NOB_ERROR, "Additional arguments to command `%s` are not allowed.", command);
        usage(program, true);
        exit(1);
    }
}

void build_dplc(bool debug_build)
{
    Nob_Cmd cmd = {0};
    cmd.count = 0;
    nob_cmd_append(&cmd, "gcc");
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");
    nob_cmd_append(&cmd, "-I./include/");
    if (debug_build)
    {
        nob_cmd_append(&cmd, "-DDPL_LEAKCHECK");
    }
    nob_cmd_append(&cmd,
                   "./src/dpl.c",
                   "./src/binding.c",
                   "./src/externals.c",
                   "./src/generator.c",
                   "./src/lexer.c",
                   "./src/parser.c",
                   "./src/program.c",
                   "./src/symbols.c",
                   "./src/value.c",
                   "./src/vm.c",
                   "./dplc.c", );
    nob_cmd_append(&cmd, "-lm");
    nob_cmd_append(&cmd, "-o", DPLC_OUTPUT);

    bool success = nob_cmd_run_sync(cmd);
    nob_cmd_free(cmd);
    if (!success)
    {
        exit(1);
    }
}

void build_dpl(bool debug_build)
{
    Nob_Cmd cmd = {0};
    cmd.count = 0;
    nob_cmd_append(&cmd, "gcc");
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");
    nob_cmd_append(&cmd, "-I./include/");
    if (debug_build)
    {
        nob_cmd_append(&cmd, "-DDPL_LEAKCHECK");
    }
    nob_cmd_append(&cmd,
                   "./src/program.c",
                   "./src/externals.c",
                   "./src/value.c",
                   "./src/vm.c",
                   "./dpl.c", );
    nob_cmd_append(&cmd, "-lm");
    nob_cmd_append(&cmd, "-o", DPL_OUTPUT);

    bool success = nob_cmd_run_sync(cmd);
    if (!success)
    {
        exit(1);
    }

    nob_cmd_free(cmd);
}

void build(Nob_String_View program, int *argc, char ***argv)
{
    nob_mkdir_if_not_exists(BUILD_DIR);

    bool have_built = false;
    bool debug_build = false;
    while (*argc > 0)
    {
        Nob_String_View target = nob_sv_shift_args(argc, argv);
        if (nob_sv_eq(target, COMMAND_DELIM))
        {
            break;
        }

        if (nob_sv_eq(target, nob_sv_from_cstr("--debug")))
        {
            if (have_built)
            {
                nob_log(NOB_ERROR, "Flag --debug cannot be set after a target has already been built.\n", target);
                usage(program, true);
                exit(1);
            }
            debug_build = true;
            continue;
        }

        if (nob_sv_eq(target, TARGET_DPLC))
        {
            build_dplc(debug_build);
            have_built = true;
        }
        else if (nob_sv_eq(target, TARGET_DPL))
        {
            build_dpl(debug_build);
            have_built = true;
        }
        else
        {
            nob_log(NOB_ERROR, "Unknown build target \"" SV_Fmt "\".\n", target);
            usage(program, true);
            exit(1);
        }
    }

    if (!have_built)
    {
        build_dplc(debug_build);
        build_dpl(debug_build);
    }
}

void cmd(Nob_String_View program, int *argc, char ***argv)
{
    Nob_Cmd cmd = {0};
    cmd.count = 0;

    Nob_String_View target = nob_sv_shift_args(argc, argv);
    if (nob_sv_eq(target, TARGET_DPLC))
    {
        nob_cmd_append(&cmd, DPLC_OUTPUT);
    }
    else if (nob_sv_eq(target, TARGET_DPL))
    {
        nob_cmd_append(&cmd, DPL_OUTPUT);
    }
    else
    {
        nob_log(NOB_ERROR, "Unknown build target \"" SV_Fmt "\".\n", target);
        usage(program, true);
        exit(1);
    }

    while (*argc > 0)
    {
        const char *arg = nob_shift_args(argc, argv);
        if (nob_sv_eq(nob_sv_from_cstr(arg), COMMAND_DELIM))
        {
            break;
        }
        nob_cmd_append(&cmd, arg);
    }

    bool success = nob_cmd_run_sync(cmd);
    if (!success)
    {
        exit(1);
    }

    nob_cmd_free(cmd);
}

void help(Nob_String_View program, int *argc, char ***argv)
{
    check_command_end(program, argc, argv, "help");

    usage(program, false);
    printf(
        "\n"
        "Runs one or more commands to build or execute DPL binaries. If \n"
        "several commands should be executed, they have to be separated \n"
        "via the \"--\" argument (see examples below).\n"
        "\n"
        "Commands:\n"
        "* build: Build the listed targets. If no targets are given, build all\n"
        "         of them.\n"
        "* cmd  : Execute the given target. Pass the following arguments (up\n"
        "         until a possible \"--\" delimiter) to the spawned process.\n"
        "* help : Print this help message.\n"
        "* run  : Run the given dpl file. Uses the targets dplc and dpl for\n"
        "         compiling and running.\n"
        "\n"
        "Targets:\n"
        "* dpl  : The DPL Virtual Machine. Can be used to run program files\n"
        "         (*.dplp).\n"
        "* dplc : The DPL Compiler. Can be used to create program files\n"
        "         (*.dplp) from source files (*.dpl).\n"
        "\n"
        "Examples:\n"
        "* Show this help:\n"
        "    " SV_Fmt " help\n"
        "* Build all executables and then compile and run the arithmetics.dpl\n"
        "  example:\n"
        "    " SV_Fmt " build -- run examples/arithmetics.dpl\n"
        "* Build the compiler and the virtual machine, run the compiler and\n"
        "  then the virtual machine on the compiled file:\n"
        "    " SV_Fmt " build dplc dpl -- cmd dplc examples/arithmetics.dpl --\n"
        "      cmd dpl examples/arithmetics.dplp\n",
        SV_Arg(program), SV_Arg(program), SV_Arg(program));
}

void build_dplc_output(Nob_String_Builder *output_path, const char *dpl_file)
{
    Nob_String_View input_file = nob_sv_filename_of(nob_sv_from_cstr(dpl_file));
    nob_sb_append_cstr(output_path, "." NOB_PATH_DELIM_STR BUILD_DIR NOB_PATH_DELIM_STR);
    nob_sb_append_sv(output_path, input_file);
    nob_sb_append_cstr(output_path, "p");
    nob_sb_append_null(output_path);
}

void run(Nob_String_View program, int *argc, char ***argv)
{
    if (*argc == 0)
    {
        nob_log(NOB_ERROR, "No program provided for command `run`.");
        usage(program, true);
        exit(1);
    }

    bool debug = false;
    bool trace = false;

    const char *program_or_flag = nob_shift_args(argc, argv);
    while (*argc > 0)
    {
        if (strcmp(program_or_flag, "-d") == 0)
        {
            debug = true;
        }
        else if (strcmp(program_or_flag, "-t") == 0)
        {
            trace = true;
        }
        else
        {
            break;
        }

        program_or_flag = nob_shift_args(argc, argv);
    }
    check_command_end(program, argc, argv, "run");

    Nob_String_Builder output_path = {0};
    build_dplc_output(&output_path, program_or_flag);

    Nob_Cmd cmd = {0};
    {
        cmd.count = 0;

        nob_cmd_append(&cmd, DPLC_OUTPUT);
        if (debug)
        {
            nob_cmd_append(&cmd, "-d");
        }
        nob_cmd_append(&cmd, "-o", output_path.items);
        nob_cmd_append(&cmd, program_or_flag);

        bool success = nob_cmd_run_sync(cmd);
        if (!success)
        {
            exit(1);
        }
    }

    {
        cmd.count = 0;

        nob_cmd_append(&cmd, DPL_OUTPUT);
        if (debug)
        {
            nob_cmd_append(&cmd, "-d");
        }
        if (trace)
        {
            nob_cmd_append(&cmd, "-t");
        }
        nob_cmd_append(&cmd, output_path.items);

        bool success = nob_cmd_run_sync(cmd);
        if (!success)
        {
            exit(1);
        }
    }

    nob_cmd_free(cmd);
    nob_sb_free(output_path);
}

typedef struct
{
    int successes;
    int failures;
    int count;
} TestResults;

void run_test_cmd(Nob_Cmd cmd, Nob_String_View test_filename, const char *test_outpath, bool record, TestResults *test_results)
{
    Nob_String_Builder output = {0};
    if (!nob_cmd_capture_sync(cmd, &output))
        exit(1);

    if (record)
    {
        if (!nob_write_entire_file(test_outpath, output.items, output.count))
            exit(1);
    }
    else
    {
        if (!nob_file_exists(test_outpath))
        {
            nob_log(NOB_ERROR, "Test output file `%s` does not exist.", test_outpath);
            exit(1);
        }

        Nob_String_Builder expected_output = {0};
        if (!nob_read_entire_file(test_outpath, &expected_output))
            exit(1);

        Nob_String_View output_view = nob_sv_from_parts(output.items, output.count);
        Nob_String_View expected_output_view = nob_sv_from_parts(expected_output.items, expected_output.count);

        size_t line_count = 0;
        bool failed = false;
        while (output_view.count > 0 && expected_output_view.count > 0)
        {
            Nob_String_View line = nob_sv_trim(nob_sv_chop_by_delim(&output_view, '\n'));
            Nob_String_View expected_line = nob_sv_trim(nob_sv_chop_by_delim(&expected_output_view, '\n'));
            line_count++;
            if (!nob_sv_eq(line, expected_line))
            {
                nob_log(NOB_WARNING, "Failure in test `" SV_Fmt "`, line %zu:", SV_Arg(test_filename), line_count);
                nob_log(NOB_WARNING, "  expected `" SV_Fmt "`,", SV_Arg(expected_line));
                nob_log(NOB_WARNING, "       got `" SV_Fmt "`.", SV_Arg(line));
                failed = true;
                break;
            }
        }

        if (!failed)
        {
            if (output_view.count > 0)
            {
                nob_log(NOB_WARNING, "Failure in test `" SV_Fmt "`: Actual output has too many lines.",
                        SV_Arg(test_filename));
                failed = true;
            }
            else if (expected_output_view.count > 0)
            {
                nob_log(NOB_WARNING, "Failure in test `" SV_Fmt "`: Actual output has too few lines.",
                        SV_Arg(test_filename));
                failed = true;
            }
        }

        if (failed)
        {
            test_results->failures++;
        }
        else
        {
            test_results->successes++;
        }

        test_results->count++;
    }
}

void run_test(Nob_String_View test_filename, bool record, TestResults *test_results)
{
    if (!record && !test_results)
    {
        nob_log(NOB_ERROR, "Cannot perform tests: neither `record` nor `test_results` is set.");
        exit(1);
    }
    nob_log(NOB_INFO, "Test: " SV_Fmt, SV_Arg(test_filename));

    Nob_String_Builder test_filepath = {0};
    nob_sb_append_cstr(&test_filepath, "." NOB_PATH_DELIM_STR "tests" NOB_PATH_DELIM_STR);
    nob_sb_append_sv(&test_filepath, test_filename);
    nob_sb_append_null(&test_filepath);

    Nob_String_Builder test_outpath = {0};
    nob_sb_append_cstr(&test_outpath, "." NOB_PATH_DELIM_STR "tests" NOB_PATH_DELIM_STR);
    nob_sb_append_sv(&test_outpath, test_filename);
    nob_sb_append_cstr(&test_outpath, ".out");
    nob_sb_append_null(&test_outpath);

    Nob_String_View extension = nob_sv_last_part_by_delim(test_filename, '.');

    if (nob_sv_eq(extension, nob_sv_from_cstr("dpl")))
    {
        Nob_String_Builder test_dplppath = {0};
        build_dplc_output(&test_dplppath, test_filepath.items);

        Nob_Cmd cmd = {0};
        {
            cmd.count = 0;
            nob_cmd_append(&cmd, DPLC_OUTPUT);
            nob_cmd_append(&cmd, "-o", test_dplppath.items);
            nob_cmd_append(&cmd, test_filepath.items);
            if (!nob_cmd_run_sync(cmd))
                exit(1);

            cmd.count = 0;
            nob_cmd_append(&cmd, DPL_OUTPUT);
            nob_cmd_append(&cmd, test_dplppath.items);

            run_test_cmd(cmd, test_filename, test_outpath.items, record, test_results);
        }
        nob_cmd_free(cmd);

        nob_sb_free(test_dplppath);
    }
    else if (nob_sv_eq(extension, nob_sv_from_cstr("c")))
    {
        Nob_String_Builder test_exepath = {0};
        nob_sb_append_cstr(&test_exepath, "." NOB_PATH_DELIM_STR BUILD_DIR NOB_PATH_DELIM_STR);
        nob_sb_append_sv(&test_exepath, test_filename);
        nob_sb_append_cstr(&test_exepath, ".exe");
        nob_sb_append_null(&test_exepath);

        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, "gcc");
        nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");
        nob_cmd_append(&cmd, "-I./include/");
        nob_cmd_append(&cmd,
                       test_filepath.items,
                       "./src/externals.c",
                       "./src/program.c",
                       "./src/value.c",
                       "./src/vm.c",
                       "./src/symbols.c");
        nob_cmd_append(&cmd, "-lm");
        nob_cmd_append(&cmd, "-o", test_exepath.items);
        nob_cmd_run_sync(cmd);

        cmd.count = 0;
        nob_cmd_append(&cmd,
                       test_exepath.items);

        run_test_cmd(cmd, test_filename, test_outpath.items, record, test_results);

        nob_cmd_free(cmd);
        nob_sb_free(test_exepath);
    }

    nob_sb_free(test_filepath);
    nob_sb_free(test_outpath);
}

void test(Nob_String_View program, int *argc, char ***argv)
{
    DIR *dfd;
    if ((dfd = opendir("." NOB_PATH_DELIM_STR "tests")) == NULL)
    {
        nob_log(NOB_ERROR, "Cannot iterate test files.");
        exit(1);
    }

    bool record = false;

    if (*argc > 0)
    {
        const char *arg = nob_shift_args(argc, argv);
        if (strcmp(arg, "-r") == 0)
        {
            record = true;
        }
    }
    check_command_end(program, argc, argv, "test");

    TestResults test_results = {0};
    struct dirent *dp;
    while ((dp = readdir(dfd)) != NULL)
    {
        Nob_String_View test_filename = nob_sv_from_cstr(dp->d_name);
        if ((test_filename.count > 0 && test_filename.data[0] == '.') ||
            (test_filename.count >= 4 && memcmp(test_filename.data + test_filename.count - 4, ".out", 4) == 0))
        {
            continue;
        }
        run_test(test_filename, record, &test_results);
    }
    closedir(dfd);

    nob_log(NOB_INFO, "Tests performed: %d", test_results.count);
    nob_log(NOB_INFO, "Tests succeeded: %d", test_results.successes);
    nob_log(NOB_INFO, "   Tests failed: %d", test_results.failures);

    if (test_results.failures > 0)
        exit(1);
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_String_View program = nob_sv_filename_of(nob_sv_shift_args(&argc, &argv));
    printf(SV_Fmt " - Buildtool for dasd.ware Programming Language (DPL)\n", SV_Arg(program));

    if (argc == 0)
    {
        nob_log(NOB_ERROR, "No command given. At least one command is needed.");
        usage(program, true);
        exit(1);
    }

    while (argc > 0)
    {
        Nob_String_View command = nob_sv_shift_args(&argc, &argv);
        if (nob_sv_eq(command, COMMAND_BUILD))
        {
            build(program, &argc, &argv);
        }
        else if (nob_sv_eq(command, COMMAND_CMD))
        {
            cmd(program, &argc, &argv);
        }
        else if (nob_sv_eq(command, COMMAND_HELP))
        {
            help(program, &argc, &argv);
        }
        else if (nob_sv_eq(command, COMMAND_RUN))
        {
            run(program, &argc, &argv);
        }
        else if (nob_sv_eq(command, COMMAND_TEST))
        {
            test(program, &argc, &argv);
        }
        else
        {
            nob_log(NOB_ERROR, "Unknown command \"" SV_Fmt "\".", SV_Arg(command));
            usage(program, true);
            exit(1);
        }
    }

    return 0;
}