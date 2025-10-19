#include "raylib.h"

#ifdef DPL_LEAKCHECK
#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"
#endif

#define NOB_IMPLEMENTATION
#include <nob.h>
#include <nobx.h>

int main(int argc, char** argv)
{
    const char* program_name = nob_shift_args(&argc, &argv);
    if (argc == 0)
    {
        fprintf(stderr, "Usage: %s <program file>.\n", program_name);
        fprintf(stderr, "No program file given.\n");
        return 1;
    }

    return 0;
}
