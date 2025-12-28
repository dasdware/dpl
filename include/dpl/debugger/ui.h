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

#define DPLG_MEMORYENTRY_HEIGHT 24

#define DPLG_STACKENTRY_HEIGHT (DPLG_TEXT_HEIGHT + 2 * DPLG_TEXT_PADDING)

typedef struct {
    Nob_String_Builder sb;
    Rectangle content;
    Rectangle view;
    Vector2 scroll;
    DPL_VirtualMachine *vm;
    int active_instruction;
} DPLG_UI_InstructionsState;

int dplg_ui_find_active_instruction(const DPLG_Instructions* instructions, const DPLG_UI_InstructionsState* state);
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

typedef enum
{
    STACK_ENTRY_UNCHANGED,
    STACK_ENTRY_ADDED,
    STACK_ENTRY_CHANGED,
    STACK_ENTRY_DELETED,
} DPLG_UI_StackEntryState;

typedef enum
{
    STACK_ENTRY_VALUE,
    STACK_ENTRY_CALL_FRAME,
} DPLG_UI_StackEntryKind;

typedef struct
{
    DPLG_UI_StackEntryKind kind;
    DPLG_UI_StackEntryState state;
    Rectangle bounds;
    union
    {
        DPL_Value value;
        DPL_CallFrame call_frame;
    } as;
} DPLG_UI_StackEntry;

typedef struct
{
    DPLG_UI_StackEntry* items;
    size_t count;
    size_t capacity;
} DPLG_UI_StackEntries;

typedef struct
{
    DPLG_UI_StackEntries entries;
    Rectangle bounds;
    Vector2 scroll;
} DPLG_UI_StackState;

void dplg_ui_stack_calculate(const DPL_VirtualMachine* vm, DPLG_UI_StackState* stack);
void dplg_ui_stack(const Rectangle bounds, DPLG_UI_StackState* stack);

typedef struct
{
    size_t id;
    size_t size;
    size_t capacity;
    size_t ref_count;
    DPL_MemoryValue* value;
} DPLG_UI_MemoryState_Item;

typedef enum
{
    MEMORY_ALLOCATED,
    MEMORY_FREE,
} DPLG_UI_MemoryState_Kind;

typedef struct
{
    DPLG_UI_MemoryState_Item* items;
    size_t count;
    size_t capacity;

    DPLG_UI_MemoryState_Kind kind;
    Rectangle bounds;
    Vector2 scroll;
} DPLG_UI_MemoryState;

void dplg_ui_memory_calculate(const DPL_VirtualMachine* vm, DPLG_UI_MemoryState* memory);
void dplg_ui_memory(const Rectangle bounds, DPLG_UI_MemoryState* stack);

#endif // __DPL_DEBUGGER_UI_H