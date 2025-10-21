#include <dpl/debugger/ui.h>

#include <raylib.h>
#include <raygui.h>
#include <rllayout.h>

static void dplg_ui__sb_append_sv_escaped(Nob_String_Builder* sb, Nob_String_View sv)
{
    nob_sb_append_cstr(sb, "\"");

    const char* pos = (char*)sv.data;
    for (size_t i = 0; i < sv.count; ++i)
    {
        switch (*pos)
        {
        case '\n':
            nob_sb_append_cstr(sb, "\\n");
            break;
        case '\r':
            nob_sb_append_cstr(sb, "\\r");
            break;
        case '\t':
            nob_sb_append_cstr(sb, "\\t");
            break;
        default:
            nob_sb_appendf(sb, "%c", *pos);
        }
        ++pos;
    }

    nob_sb_append_cstr(sb, "\"");
}

static void dplg_ui__append_parameter(Nob_String_Builder* sb, const DPLG_Instruction_Parameter parameter)
{
    if (parameter.kind == PARAMETER_EMPTY)
    {
        return;
    }

    nob_sb_append_cstr(sb, " ");

    switch (parameter.kind)
    {
    case PARAMETER_NUMBER:
        nob_sb_appendf(sb, "[%f]", parameter.as.number);
        return;
    case PARAMETER_BOOLEAN:
        nob_sb_append_cstr(sb, parameter.as.boolean ? "[true]" : "false");
        return;
    case PARAMETER_STRING:
        nob_sb_append_cstr(sb, "[");
        dplg_ui__sb_append_sv_escaped(sb, parameter.as.string);
        nob_sb_append_cstr(sb, "]");
        return;
    case PARAMETER_SIZE:
        nob_sb_appendf(sb, "[%zu]", parameter.as.size);
        return;
    case PARAMETER_CSTR:
        nob_sb_append_cstr(sb, "[");
        dplg_ui__sb_append_sv_escaped(sb, nob_sv_from_cstr(parameter.as.cstr));
        nob_sb_append_cstr(sb, "]");
    default:
    }
}

static void dplg_ui__instruction_item(DPLG_Instruction* instruction, DPLG_UI_InstructionsState* state)
{
    state->sb.count = 0;
    nob_sb_append_cstr(&state->sb, instruction->name);
    dplg_ui__append_parameter(&state->sb, instruction->parameter0);
    dplg_ui__append_parameter(&state->sb, instruction->parameter1);
    nob_sb_append_null(&state->sb);

    const Rectangle bounds = {
        .x = instruction->bounds.x + state->view.x + state->scroll.x,
        .y = instruction->bounds.y + state->view.y + state->scroll.y,
        .width = state->view.width,
        .height = instruction->bounds.height,
    };
    LayoutBeginRectangle(RLD_DEFAULT, LayoutPaddingSymmetric(bounds, 4, 4));
    {
        LayoutBeginStack(RLD_DEFAULT, DIRECTION_HORIZONTAL, 0, 8);
        {
            int old_state = GuiGetState();
            GuiToggle(LayoutRectangle(RL_SIZE(DPLG_INSTRUCTION_HEIGHT)), "#132#", &instruction->is_breakpoint);

            if (instruction->is_breakpoint)
            {
                GuiSetState(STATE_PRESSED);
            }
            else
            {
                GuiSetState(STATE_NORMAL);
            }

            GuiLabel(LayoutRectangle(RL_SIZE(40)), TextFormat("%zu", instruction->ip));
            GuiLabel(LayoutRectangle(RLD_REMAINING), state->sb.items);

            GuiSetState(old_state);
        }
        LayoutEnd();
    }
    LayoutEnd();
}

void dplg_ui_instructions(const DPLG_Instructions* instructions, Rectangle bounds, DPLG_UI_InstructionsState* state)
{
    GuiGroupBox(LayoutPadding(bounds, 10, 20, 10, 10), "Instructions");
    const Rectangle insets = LayoutPadding(bounds, 20, 30, 20, 20);

    GuiScrollPanel(insets, NULL, state->content, &state->scroll, &state->view);
    BeginScissorMode(state->view.x, state->view.y, state->view.width,
                     state->view.height);
    for (size_t i = 0; i < instructions->count; i++)
    {
        dplg_ui__instruction_item(&instructions->items[i], state);
    }
    EndScissorMode();
}
