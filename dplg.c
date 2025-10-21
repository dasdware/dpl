#include "raylib.h"

#ifdef DPL_LEAKCHECK
#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"
#endif

#include <dpl/program.h>

#include <dpl/debugger/instructions.h>
#include <dpl/debugger/ui.h>

#define DW_BYTEBUFFER_IMPLEMENTATION
#include <dw_byte_buffer.h>

#define DW_MEMORY_TABLE_IMPLEMENTATION
#include <dw_memory_table.h>

#define RL_GUIEXT_IMPLEMENTATION
#include <rlguiext.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#define RLLAYOUT_IMPLEMENTATION
#include <rllayout.h>

#define NOB_IMPLEMENTATION
#include <nob.h>
#include <nobx.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"

int main(int argc, char** argv)
{
    const char* program_name = nob_shift_args(&argc, &argv);
    if (argc == 0)
    {
        fprintf(stderr, "Usage: %s <program file>.\n", program_name);
        fprintf(stderr, "No program file given.\n");
        return 1;
    }

    const char* program_to_run = nob_shift_args(&argc, &argv);
    DPL_Program program = {0};
    dplp_load(&program, program_to_run);

    DPLG_Instructions instructions = {0};
    dplg_load_instructions(&instructions, &program);

    DPLG_UI_InstructionsState instructions_state = {
        .content = {
            .x = 0,
            .y = 0,
            .width = DPLG_INSTRUCTION_WIDTH,
            .height = instructions.count * DPLG_INSTRUCTION_HEIGHT
        },
    };

    const char* window_title = nob_temp_sprintf("%s - DPL Debugger", program_to_run);
    InitWindow(DPLG_SCREEN_WIDTH, DPLG_SCREEN_HEIGHT, window_title);

    SetTargetFPS(60); // Set our game to run at 60 frames-per-second
    while (!WindowShouldClose())
    {
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);

            LayoutBeginScreen(0);
            {
                LayoutBeginAnchored(RLD_DEFAULT, 5);

                dplg_ui_instructions(
                    &instructions,
                    LayoutRectangle(RL_ANCHOR_LEFT(DPLG_INSTRUCTION_WIDTH + 2 * 20)),
                    &instructions_state);

                LayoutEnd();
            }
            LayoutEnd();
        }
        EndDrawing();
    }

    CloseWindow();

    nob_sb_free(instructions_state.sb);
    dplp_free(&program);
    return 0;
}
