#include <dpl/debugger/ui.h>

#include <raylib.h>
#include <raygui.h>
#include <rllayout.h>
#include <math.h>

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

static void dplg_ui__append_value(Nob_String_Builder* sb, DPL_Value value);

static void dplg_ui__append_value_number(Nob_String_Builder* sb, double value)
{
    nob_sb_appendf(sb, "[%s: ", dpl_value_kind_name(VALUE_NUMBER));

    const double abs_value = fabs(value);
    if (abs_value - floorf(abs_value) < DPL_VALUE_EPSILON)
    {
        nob_sb_appendf(sb, "%i", (int)round(value));
    }
    else
    {
        nob_sb_appendf(sb, "%f", value);
    }

    nob_sb_append_cstr(sb, "]");
}

static void dplg_ui__append_value_string(Nob_String_Builder* sb, const Nob_String_View value)
{
    nob_sb_appendf(sb, "[%s: ", dpl_value_kind_name(VALUE_STRING));
    dplg_ui__sb_append_sv_escaped(sb, value);
    nob_sb_append_cstr(sb, "]");
}

static void dplg_ui__append_value_boolean(Nob_String_Builder* sb, const bool value)
{
    nob_sb_appendf(sb, "[%s: %s]", dpl_value_kind_name(VALUE_BOOLEAN), value ? "true" : "false");
}

static void dplg_ui__append_value_object(Nob_String_Builder* sb, DW_MemoryTable_Item* object)
{
    const uint8_t field_count = dpl_value_object_field_count(object);
    nob_sb_appendf(sb, "[%s(%d): ", dpl_value_kind_name(VALUE_OBJECT), field_count);
    for (uint8_t field_index = 0; field_index < field_count; ++field_index)
    {
        dplg_ui__append_value(sb, dpl_value_object_get_field(object, field_index));
    }
    nob_sb_append_cstr(sb, "]");
}

static void dplg_ui__append_value_array(Nob_String_Builder* sb, DW_MemoryTable_Item* array)
{
    if (array == NULL)
    {
        nob_sb_appendf(sb, "[%s slot]", dpl_value_kind_name(VALUE_ARRAY));
        return;
    }

    const uint8_t element_count = dpl_value_array_element_count(array);
    nob_sb_appendf(sb, "[%s(%d): ", dpl_value_kind_name(VALUE_ARRAY), element_count);
    for (uint8_t element_index = 0; element_index < element_count; ++element_index)
    {
        dplg_ui__append_value(sb, dpl_value_array_get_element(array, element_index));
    }
    nob_sb_append_cstr(sb, "]");
}

static void dplg_ui__append_value(Nob_String_Builder* sb, const DPL_Value value)
{
    switch (value.kind)
    {
    case VALUE_NUMBER:
        dplg_ui__append_value_number(sb, value.as.number);
        break;
    case VALUE_STRING:
        dplg_ui__append_value_string(sb, value.as.string);
        break;
    case VALUE_BOOLEAN:
        dplg_ui__append_value_boolean(sb, value.as.boolean);
        break;
    case VALUE_OBJECT:
        dplg_ui__append_value_object(sb, value.as.object);
        break;
    case VALUE_ARRAY:
        dplg_ui__append_value_array(sb, value.as.array);
        break;
    default:
        DW_UNIMPLEMENTED_MSG("Cannot debug print value of kind `%s`.",
                             dpl_value_kind_name(value.kind));
    }
}

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
    if (instruction->parameter_count > 0)
    {
        nob_sb_append_cstr(&state->sb," ");
        dplg_ui__append_value(&state->sb, instruction->parameter0);
    }
    if (instruction->parameter_count > 1)
    {
        nob_sb_append_cstr(&state->sb," ");
        dplg_ui__append_value(&state->sb, instruction->parameter1);
    }
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

void dplg_ui_terminal(const Rectangle bounds, DPLG_UI_TerminalState* state)
{
    dplg_ui__begin_titled_group(bounds, "Terminal", state->content, &state->view, &state->scroll);
    for (size_t i = 0; i < state->lines.count; i++)
    {
        const Rectangle line_bounds = {
            .x = state->view.x + state->scroll.x + DPLG_TEXT_PADDING,
            .y = state->view.y + state->scroll.y + i * DPLG_TERMINAL_LINE_HEIGHT,
            .width = state->content.width,
            .height = DPLG_TERMINAL_LINE_HEIGHT,
        };
        GuiLabel(line_bounds, TextFormat(SV_Fmt, SV_Arg(state->lines.items[i])));
    }
    dplg_ui__end_titled_group();
}

int dplg_ui_terminal_append(void* state, const char* str, ...)
{
    DPLG_UI_TerminalState* terminal_state = state;
    va_list args;

    va_start(args, str);
    const int n = vsnprintf(NULL, 0, str, args);
    va_end(args);

    // NOTE: the new_capacity needs to be +1 because of the null terminator.
    // However, further below we increase sb->count by n, not n + 1.
    // This is because we don't want the sb to include the null terminator. The user can always sb_append_null() if they want it
    nob_da_reserve(&terminal_state->sb, terminal_state->sb.count + n + 1);
    char *dest = terminal_state->sb.items + terminal_state->sb.count;
    va_start(args, str);
    vsnprintf(dest, n + 1, str, args);
    va_end(args);

    terminal_state->sb.count += n;

    terminal_state->content.width = 0;
    terminal_state->content.height = 0;

    terminal_state->lines.count = 0;
    Nob_String_View lines = nob_sv_from_parts(terminal_state->sb.items, terminal_state->sb.count);

    while (lines.count > 0)
    {
        const Nob_String_View line = nob_sv_chop_by_delim(&lines, '\n');
        nob_da_append(&terminal_state->lines, line);
        terminal_state->content.width = max(
            terminal_state->content.width,
            MeasureText(TextFormat(SV_Fmt, SV_Arg(line)), DPLG_TEXT_HEIGHT) + 2 * DPLG_TEXT_PADDING);
        terminal_state->content.height += DPLG_TERMINAL_LINE_HEIGHT;
    }

    terminal_state->scroll.y = 1000*1000.f;

    return n;
}

void dplg_ui_stack_calculate(const DPL_VirtualMachine* vm, DPLG_UI_StackState* stack)
{
    stack->entries.count = 0;

    size_t offset = 0;
    for (size_t i = 0; i < vm->stack_top; ++i)
    {
        const DPL_Value value = vm->stack[i];
        size_t height = DPLG_STACKENTRY_HEIGHT;

        const DPLG_UI_StackEntry entry = {
            .value = value,
            .bounds = {
                .x = 0,
                .y = offset,
                .width = 500,
                .height = height,
            }
        };
        nob_da_append(&stack->entries, entry);

        offset += height;
    }

    stack->bounds = (Rectangle) {
        .x = 0,
        .y = 0,
        .width = 500,
        .height = offset,
    };
}

void dplg_ui_stack(const Rectangle bounds, DPLG_UI_StackState* stack)
{
    Nob_String_Builder sb = {0};

    Rectangle view;
    dplg_ui__begin_titled_group(
        bounds,
        "Stack",
        (Rectangle) {
            .width = bounds.width,
            .height = stack->bounds.height,
        },
        &view,
        &stack->scroll
    );

    view = LayoutPaddingAll(view, 2);
    for (size_t i = 0; i < stack->entries.count; i++)
    {
        const DPLG_UI_StackEntry entry = stack->entries.items[i];
        const Rectangle view_bounds = LayoutPaddingSymmetric(((Rectangle) {
            .x = view.x + entry.bounds.x + stack->scroll.x,
            .y = view.y + entry.bounds.y + stack->scroll.y,
            .width = view.width,
            .height = entry.bounds.height,
        }), 0, 1);

        sb.count = 0;
        nob_sb_appendf(&sb, "#%02llu ", i);
        dplg_ui__append_value(&sb, entry.value);
        nob_sb_append_null(&sb);

        DrawRectangleRec(view_bounds, GetColor(0x3E4550FF));
        GuiLabel(LayoutPaddingAll(view_bounds, 4), sb.items);
    }

    dplg_ui__end_titled_group();
}