#define NOB_IMPLEMENTATION
#include "./include/nob.h"

#define DPLC_OUTPUT "dplc.exe"
#define DPL_OUTPUT "dpl.exe"

#define COMMAND_DELIM nob_sv_from_cstr("--")

#define COMMAND_BUILD nob_sv_from_cstr("build")
#define COMMAND_CMD nob_sv_from_cstr("cmd")
#define COMMAND_HELP nob_sv_from_cstr("help")
#define COMMAND_RUN nob_sv_from_cstr("run")

#define TARGET_DPLC nob_sv_from_cstr("dplc")
#define TARGET_DPL nob_sv_from_cstr("dpl")


void usage(Nob_String_View program, bool help_hint)
{
    printf(
        "\n"
        "Usage:\n"
        SV_Fmt" command [parameters] [-- command [parameters]]*\n",
        SV_Arg(program)
    );

    if (help_hint)
    {
        printf(
            "\n"
            "Use the following command for further details:\n"
            " "SV_Fmt" help\n",
            SV_Arg(program)
        );
    }
}

void check_command_end(Nob_String_View program, int *argc, char ***argv, const char* command)
{
    if (*argc > 0 && !nob_sv_eq(nob_sv_shift_args(argc, argv), COMMAND_DELIM)) {
        nob_log(NOB_ERROR, "Additional arguments to command `%s` are not allowed.", command);
        usage(program, true);
        exit(1);
    }
}

void build_dplc(void) {
    Nob_Cmd cmd = {0};
    cmd.count = 0;
    nob_cmd_append(&cmd, "gcc");
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");
    nob_cmd_append(&cmd, "-I./include/");
    nob_cmd_append(&cmd,
                   "./src/dpl.c",
                   "./src/externals.c",
                   "./src/program.c",
                   "./src/value.c",
                   "./src/vm.c",
                   "./dplc.c",
                  );
    nob_cmd_append(&cmd, "-lm");
    nob_cmd_append(&cmd, "-o", "./"DPLC_OUTPUT);

    bool success = nob_cmd_run_sync(cmd);
    nob_cmd_free(cmd);
    if (!success) {
        exit(1);
    }
}

void build_dpl(void)
{
    Nob_Cmd cmd = {0};
    cmd.count = 0;
    nob_cmd_append(&cmd, "gcc");
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");
    nob_cmd_append(&cmd, "-I./include/");
    nob_cmd_append(&cmd,
                   "./src/program.c",
                   "./src/externals.c",
                   "./src/value.c",
                   "./src/vm.c",
                   "./dpl.c",
                  );
    nob_cmd_append(&cmd, "-lm");
    nob_cmd_append(&cmd, "-o", "./"DPL_OUTPUT);

    bool success = nob_cmd_run_sync(cmd);
    if (!success) {
        exit(1);
    }

    nob_cmd_free(cmd);
}

void build(Nob_String_View program, int *argc, char ***argv)
{
    bool have_built = false;
    while (argc > 0) {
        Nob_String_View target = nob_sv_shift_args(argc, argv);
        if (nob_sv_eq(target, COMMAND_DELIM)) {
            break;
        }

        if (nob_sv_eq(target, TARGET_DPLC)) {
            build_dplc();
            have_built = true;
        } else if (nob_sv_eq(target, TARGET_DPL)) {
            build_dpl();
            have_built = true;
        } else {
            nob_log(NOB_ERROR, "Unknown build target \""SV_Fmt"\".\n", target);
            usage(program, true);
            exit(1);
        }
    }

    if (!have_built) {
        build_dplc();
        build_dpl();
    }
}

void cmd(Nob_String_View program, int *argc, char ***argv)
{
    Nob_Cmd cmd = {0};
    cmd.count = 0;

    Nob_String_View target = nob_sv_shift_args(argc, argv);
    if (nob_sv_eq(target, TARGET_DPLC)) {
        nob_cmd_append(&cmd, "."NOB_PATH_DELIM_STR  DPLC_OUTPUT);
    } else if (nob_sv_eq(target, TARGET_DPL)) {
        nob_cmd_append(&cmd, "."NOB_PATH_DELIM_STR  DPL_OUTPUT);
    } else {
        nob_log(NOB_ERROR, "Unknown build target \""SV_Fmt"\".\n", target);
        usage(program, true);
        exit(1);
    }

    while (*argc > 0) {
        const char* arg = nob_shift_args(argc, argv);
        if (nob_sv_eq(nob_sv_from_cstr(arg), COMMAND_DELIM)) {
            break;
        }
        nob_cmd_append(&cmd, arg);
    }

    bool success = nob_cmd_run_sync(cmd);
    if (!success) {
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
        "    "SV_Fmt" help\n"
        "* Build all executables and then compile and run the arithmetics.dpl\n"
        "  example:\n"
        "    "SV_Fmt" build -- run examples/arithmetics.dpl\n"
        "* Build the compiler and the virtual machine, run the compiler and\n"
        "  then the virtual machine on the compiled file:\n"
        "    "SV_Fmt" build dplc dpl -- cmd dplc examples/arithmetics.dpl --\n"
        "      cmd dpl examples/arithmetics.dplp\n",
        SV_Arg(program), SV_Arg(program), SV_Arg(program)
    );
}

void run(Nob_String_View program, int *argc, char ***argv)
{
    if (*argc == 0) {
        nob_log(NOB_ERROR, "No program provided for command `run`.");
        usage(program, true);
        exit(1);
    }

    const char* program_to_run = nob_shift_args(argc, argv);
    check_command_end(program, argc, argv, "run");

    Nob_Cmd cmd = {0};
    {
        cmd.count = 0;

        nob_cmd_append(&cmd, "."NOB_PATH_DELIM_STR  DPLC_OUTPUT);
        nob_cmd_append(&cmd, program_to_run);

        bool success = nob_cmd_run_sync(cmd);
        if (!success) {
            exit(1);
        }
    }

    {
        cmd.count = 0;

        nob_cmd_append(&cmd, "."NOB_PATH_DELIM_STR  DPL_OUTPUT);
        nob_cmd_append(&cmd, nob_temp_change_file_ext(program_to_run, "dplp"));

        bool success = nob_cmd_run_sync(cmd);
        if (!success) {
            exit(1);
        }
    }

    nob_cmd_free(cmd);
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_String_View program = nob_sv_filename_of(nob_sv_shift_args(&argc, &argv));
    printf(SV_Fmt" - Buildtool for dasd.ware Programming Language (DPL)\n", SV_Arg(program));

    if (argc == 0) {
        nob_log(NOB_ERROR, "No command given. At least one command is needed.");
        usage(program, true);
        exit(1);
    }

    while (argc > 0)
    {
        Nob_String_View command = nob_sv_shift_args(&argc, &argv);
        if (nob_sv_eq(command, COMMAND_BUILD)) {
            build(program, &argc, &argv);
        } else if (nob_sv_eq(command, COMMAND_CMD)) {
            cmd(program, &argc, &argv);
        } else if (nob_sv_eq(command, COMMAND_HELP)) {
            help(program, &argc, &argv);
        } else if (nob_sv_eq(command, COMMAND_RUN)) {
            run(program, &argc, &argv);
        } else {
            nob_log(NOB_ERROR, "Unknown command \""SV_Fmt"\".", SV_Arg(command));
            usage(program, true);
            exit(1);
        }

    }

    return 0;
}