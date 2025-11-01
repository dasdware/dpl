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

#define DPLG_TERMINAL_LINE_HEIGHT 24

typedef struct {
    Nob_String_Builder sb;
    Rectangle content;
    Rectangle view;
    Vector2 scroll;
    DPL_VirtualMachine *vm;
    int active_instruction;
} DPLG_UI_InstructionsState;

void dplg_ui_instructions(const DPLG_Instructions* instructions, const Rectangle bounds, DPLG_UI_InstructionsState* state);

typedef struct
{
    Nob_String_View* items;
    size_t count;
    size_t capacity;
} DPLG_UI_TerminalState_Lines;

typedef struct {
    Rectangle content;
    Rectangle view;
    Vector2 scroll;

    Nob_String_Builder sb;
    DPLG_UI_TerminalState_Lines lines;
} DPLG_UI_TerminalState;

void dplg_ui_terminal(const Rectangle bounds, DPLG_UI_TerminalState* state);
int dplg_ui_terminal_append(void* state, const char* str, ...);

#endif // __DPL_DEBUGGER_UI_H