#define NOB_IMPLEMENTATION
#include "./include/nob.h"

bool build_dplc(void) {
    bool result = true;

    Nob_Cmd cmd = {0};
    cmd.count = 0;
    nob_cmd_append(&cmd, "gcc");
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");
    nob_cmd_append(&cmd, "-I./include/");
    nob_cmd_append(&cmd, "-o", "./dplc.exe");
    nob_cmd_append(&cmd,
                   "./src/dpl.c",
                   "./src/bytecode.c",
                   "./dplc.c",
                  );
    if (!nob_cmd_run_sync(cmd)) nob_return_defer(false);

defer:
    nob_cmd_free(cmd);
    return result;
}

bool run_dplc(int argc, char **argv)
{
    bool result = true;

    Nob_Cmd cmd = {0};
    cmd.count = 0;
    nob_cmd_append(&cmd, ".\\dplc.exe");
    while (argc > 0) {
        nob_cmd_append(&cmd, nob_shift_args(&argc, &argv));
    }
    if (!nob_cmd_run_sync(cmd)) nob_return_defer(false);

defer:
    nob_cmd_free(cmd);
    return result;
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *program = nob_shift_args(&argc, &argv);

    const char *subcommand = NULL;
    if (argc <= 0) {
        subcommand = "build";
    } else {
        subcommand = nob_shift_args(&argc, &argv);
    }

    if (!build_dplc()) {
        return 1;
    }
    if (!run_dplc(argc, argv)) {
        return 1;
    }

    return 0;
}