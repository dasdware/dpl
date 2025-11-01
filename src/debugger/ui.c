#include <dpl/debugger/ui.h>

#include <raylib.h>
#include <raygui.h>
#include <rllayout.h>

static void dplg_ui__begin_titled_group(const Rectangle bounds, const char* title, const Rectangle content,
                                        Rectangle* view, Vector2* scroll)
{
    GuiScrollPanel(bounds, title, content, scroll, view);
    BeginScissorMode(view->x, view->y, view->width, view->height);
}

static void dplg_ui__end_titled_group()
{
    EndScissorMode();
}


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

static int dplg_ui__find_active_instruction(const DPLG_Instructions* instructions,
                                            const DPLG_UI_InstructionsState* state)
{
    for (size_t i = 0; i < instructions->count; i++)
    {
        if (instructions->items[i].ip == state->vm->program_stream.position)
        {
            return i;
        }
    }
    return -1;
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
    if (instruction->ip == state->vm->program_stream.position)
    {
        DrawRectangleRec(bounds, DARKBLUE);
    }
    else if (instruction->is_breakpoint)
    {
        DrawRectangleRec(
            bounds,
            GetColor(GuiGetStyle(TOGGLE, BASE_COLOR_PRESSED))
        );
    }

    LayoutBeginRectangle(RLD_DEFAULT, LayoutPaddingSymmetric(bounds, 4, 4));
    {
        LayoutBeginStack(RLD_DEFAULT, DIRECTION_HORIZONTAL, 0, 8);
        {
            GuiToggle(LayoutRectangle(RL_SIZE(DPLG_INSTRUCTION_HEIGHT)), "#132#", &instruction->is_breakpoint);

            GuiLabel(LayoutRectangle(RL_SIZE(40)), TextFormat("%zu", instruction->ip));
            GuiLabel(LayoutRectangle(RLD_REMAINING), state->sb.items);
        }
        LayoutEnd();
    }
    LayoutEnd();
}

void dplg_ui_instructions(const DPLG_Instructions* instructions, const Rectangle bounds,
                          DPLG_UI_InstructionsState* state)
{
    int active_instruction = dplg_ui__find_active_instruction(instructions, state);
    if (active_instruction != state->active_instruction)
    {
        if (active_instruction == 0)
        {
            state->scroll.y = 0;
        }
        else
        {
            const Rectangle item_bounds = instructions->items[active_instruction].bounds;
            const float item_top = state->scroll.y + item_bounds.y;
            if (item_top < 0)
            {
                state->scroll.y -= item_top;
            }
            else
            {
                const float item_bottom = state->scroll.y + item_bounds.y + item_bounds.height;
                if (item_bottom > state->view.height)
                {
                    state->scroll.y -= item_bottom - state->view.height;
                }
            }
        }

        state->active_instruction = active_instruction;
    }

    dplg_ui__begin_titled_group(bounds, "Instructions", state->content, &state->view, &state->scroll);
    for (size_t i = 0; i < instructions->count; i++)
    {
        dplg_ui__instruction_item(&instructions->items[i], state);
    }
    dplg_ui__end_titled_group();
}
