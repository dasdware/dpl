#ifndef __DPL_DEBUGGER_UI_H
#define __DPL_DEBUGGER_UI_H

#include <dpl/debugger/instructions.h>
#include <dpl/vm/vm.h>

#define DPLG_SCREEN_WIDTH (16 * 90)
#define DPLG_SCREEN_HEIGHT (9 * 90)

#define DPLG_TEXT_HEIGHT 20
#define DPLG_TEXT_PADDING 6
#define DPLG_INSTRUCTION_HEIGHT (DPLG_TEXT_HEIGHT + 2 * DPLG_TEXT_PADDING)
#define DPLG_INSTRUCTION_WIDTH  400

typedef struct {
    Nob_String_Builder sb;
    Rectangle content;
    Rectangle view;
    Vector2 scroll;
    DPL_VirtualMachine *vm;
    int active_instruction;
} DPLG_UI_InstructionsState;

void dplg_ui_instructions(const DPLG_Instructions* instructions, Rectangle bounds, DPLG_UI_InstructionsState* state);

#endif // __DPL_DEBUGGER_UI_H