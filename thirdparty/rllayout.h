#ifndef __RL_LAYOUT_H
#define __RL_LAYOUT_H

#ifndef RLLAYOUT_STACK_CAPACITY
#   define RLLAYOUT_STACK_CAPACITY 32
#endif

#ifndef RLLAYOUT_SIZE_BITS
#   define RLLAYOUT_SIZE_BITS 24
#endif

#define RL_SIZE_MASK ((1 << RLLAYOUT_SIZE_BITS) - 1)

#define RL_SIZE(data) ((data) & RL_SIZE_MASK)

#define RL_FLAG_DEFAULT       (0x00 << RLLAYOUT_SIZE_BITS)
#define RL_FLAG_ANCHOR_TOP    (0x01 << RLLAYOUT_SIZE_BITS)
#define RL_FLAG_ANCHOR_LEFT   (0x02 << RLLAYOUT_SIZE_BITS)
#define RL_FLAG_ANCHOR_BOTTOM (0x03 << RLLAYOUT_SIZE_BITS)
#define RL_FLAG_ANCHOR_RIGHT  (0x04 << RLLAYOUT_SIZE_BITS)
#define RL_FLAG_OPPOSITE      (0x05 << RLLAYOUT_SIZE_BITS)
#define RL_FLAG_REMAINING     (0x06 << RLLAYOUT_SIZE_BITS)

#define RL_DEFAULT(size) (RL_FLAG_DEFAULT | (size))
#define RL_ANCHOR_TOP(size) (RL_FLAG_ANCHOR_TOP | (size))
#define RL_ANCHOR_LEFT(size) (RL_FLAG_ANCHOR_LEFT | (size))
#define RL_ANCHOR_BOTTOM(size) (RL_FLAG_ANCHOR_BOTTOM | (size))
#define RL_ANCHOR_RIGHT(size) (RL_FLAG_ANCHOR_RIGHT | (size))
#define RL_OPPOSITE(size) (RL_FLAG_OPPOSITE | (size))
#define RL_REMAINING(size) (RL_FLAG_REMAINING | (size))

#define RLD_DEFAULT RL_DEFAULT(0)
#define RLD_OPPOSITE RL_OPPOSITE(0)
#define RLD_REMAINING RL_REMAINING(0)

#define RL_IS_DEFAULT(data) (((data) & ~RL_SIZE_MASK) == RL_FLAG_DEFAULT)
#define RL_IS_ANCHOR_TOP(data) (((data) & ~RL_SIZE_MASK) == RL_FLAG_ANCHOR_TOP)
#define RL_IS_ANCHOR_LEFT(data) (((data) & ~RL_SIZE_MASK) == RL_FLAG_ANCHOR_LEFT)
#define RL_IS_ANCHOR_BOTTOM(data) (((data) & ~RL_SIZE_MASK) == RL_FLAG_ANCHOR_BOTTOM)
#define RL_IS_ANCHOR_RIGHT(data) (((data) & ~RL_SIZE_MASK) == RL_FLAG_ANCHOR_RIGHT)
#define RL_IS_OPPOSITE(data) (((data) & ~RL_SIZE_MASK) == RL_FLAG_OPPOSITE)
#define RL_IS_REMAINING(data) (((data) & ~RL_SIZE_MASK) == RL_FLAG_REMAINING)

#include <raylib.h>
#include <error.h>

typedef enum
{
    LAYOUT_SCREEN,
    LAYOUT_ANCHOR,
    LAYOUT_STACK,
    LAYOUT_SPACED,
    LAYOUT_RECTANGLE,
} RL_LayoutKind;

typedef enum
{
    DIRECTION_VERTICAL,
    DIRECTION_HORIZONTAL,
} RL_LayoutDirection;

typedef struct
{
    int padding;
} RL_LayoutPaddingData;

typedef struct
{
    int gap;
    int inset_left;
    int inset_top;
    int inset_right;
    int inset_bottom;
} RL_LayoutAnchorData;

typedef struct
{
    RL_LayoutDirection direction;
    int item_size;
    int gap;
    int inset_begin;
    int inset_end;
} RL_LayoutStackData;

typedef struct
{
    RL_LayoutDirection direction;
    int count;
    int gap;
    int cursor;
} RL_LayoutSpacedData;

typedef union
{
    RL_LayoutPaddingData screen;
    RL_LayoutAnchorData anchored;
    RL_LayoutStackData stack;
    RL_LayoutSpacedData spaced;
    Rectangle rectangle;
} RL_LayoutAs;

typedef struct
{
    RL_LayoutKind kind;
    Rectangle parent_bounds;
    RL_LayoutAs as;
} RL_Layout;

Rectangle LayoutPadding(Rectangle bounds, int left, int top, int right, int bottom);
#define LayoutPaddingAll(bounds, padding) LayoutPadding(bounds, padding, padding, padding, padding)
#define LayoutPaddingSymmetric(bounds, horizontal, vertical) LayoutPadding(bounds, horizontal, vertical, horizontal, vertical)

Rectangle LayoutCenter(Rectangle bounds, int width, int height);

void LayoutSpacing(int amount);

Rectangle LayoutRectangle(int data);
#define LayoutDefault() LayoutRectangle(RLD_DEFAULT)
#define LayoutOpposite() LayoutRectangle(RLD_OPPOSITE)
#define LayoutRemaining() LayoutRectangle(RLD_REMAINING)

void LayoutBeginScreen(int padding);
void LayoutBeginAnchored(int data, int gap);
void LayoutBeginStack(int data, RL_LayoutDirection direction, int item_size, int gap);
void LayoutBeginSpaced(int data, RL_LayoutDirection direction, int count, int gap);
void LayoutBeginRectangle(int data, Rectangle rectangle);
void LayoutEnd();

// Controls/Helpers
void LayoutGuiButtonLabel(const char* text);
void LayoutGuiLabel(const char* text);

#endif // __RL_LAYOUT_H

#ifdef RLLAYOUT_IMPLEMENTATION

#include <nob.h>

static RL_Layout rl_layout_stack[RLLAYOUT_STACK_CAPACITY] = {0};
static size_t rl_layout_stack_count = 0;

Rectangle LayoutPadding(Rectangle bounds, int left, int top, int right, int bottom)
{
    Rectangle result = bounds;
    result.x += left;
    result.y += top;
    result.width -= left + right;
    result.height -= top + bottom;
    return result;
}

Rectangle LayoutCenter(Rectangle bounds, int width, int height)
{
    Rectangle result = {0};
    result.x = bounds.x + (bounds.width - width) / 2;
    result.y = bounds.y + (bounds.height - height) / 2;
    result.width = width;
    result.height = height;
    return result;
}

Rectangle LayoutRectangle(int data)
{
    NOB_ASSERT(rl_layout_stack_count > 0);

    RL_Layout* layout = &rl_layout_stack[rl_layout_stack_count - 1];
    Rectangle result = layout->parent_bounds;

    switch (layout->kind)
    {
    case LAYOUT_SCREEN:
        result = CLITERAL(Rectangle) {
            .x = 0,
            .y = 0,
            .width = GetScreenWidth(),
            .height = GetScreenHeight(),
        };
        result = LayoutPaddingAll(result, layout->as.screen.padding);
        break;
    case LAYOUT_ANCHOR:
    {
        int size = RL_SIZE(data);
        if (RL_IS_ANCHOR_TOP(data)) {
            result.x += layout->as.anchored.inset_left;
            result.y += layout->as.anchored.inset_top;
            result.height = size;
            result.width -= layout->as.anchored.inset_left - layout->as.anchored.inset_right;
            layout->as.anchored.inset_top += size + layout->as.anchored.gap;
        } else if (RL_IS_ANCHOR_BOTTOM(data)) {
            result.x += layout->as.anchored.inset_left;
            result.y += result.height - layout->as.anchored.inset_bottom - size;
            result.height = size;
            result.width -= layout->as.anchored.inset_left - layout->as.anchored.inset_right;
            layout->as.anchored.inset_bottom += size + layout->as.anchored.gap;
        } else if (RL_IS_ANCHOR_LEFT(data)) {
            result.x += layout->as.anchored.inset_left;
            result.y += layout->as.anchored.inset_top;
            result.width = size;
            result.height -= layout->as.anchored.inset_top + layout->as.anchored.inset_bottom;
            layout->as.anchored.inset_left += size + layout->as.anchored.gap;
        } else if (RL_IS_ANCHOR_RIGHT(data)) {
            result.x += result.width - layout->as.anchored.inset_right - size;
            result.y += layout->as.anchored.inset_top;
            result.width = size;
            result.height -= layout->as.anchored.inset_top + layout->as.anchored.inset_bottom;
            layout->as.anchored.inset_right += size + layout->as.anchored.gap;
        } else if (RL_IS_REMAINING(data) || RL_IS_DEFAULT(data)) {
            result.x += layout->as.anchored.inset_left;
            result.y += layout->as.anchored.inset_top;
            result.width -= layout->as.anchored.inset_left + layout->as.anchored.inset_right;
            result.height -= layout->as.anchored.inset_top + layout->as.anchored.inset_bottom;
        } else {
            DW_UNIMPLEMENTED_MSG("Unknown anchor data `%d`.", data);
        }
        break;
    }
    case LAYOUT_STACK:
    {
        int item_size = RL_SIZE(data);
        if (RL_IS_REMAINING(data)) {
            if (layout->as.stack.direction == DIRECTION_HORIZONTAL) {
                item_size = result.width - layout->as.stack.inset_begin - layout->as.stack.inset_end;
            } else if (layout->as.stack.direction == DIRECTION_VERTICAL) {
                item_size = result.height - layout->as.stack.inset_begin - layout->as.stack.inset_end;
            }
        } else if (item_size == 0) {
            item_size = layout->as.stack.item_size;
        }

        switch (layout->as.stack.direction)
        {
        case DIRECTION_HORIZONTAL:
            if (RL_IS_OPPOSITE(data)) {
                result.x += result.width - layout->as.stack.inset_end - item_size;
                layout->as.stack.inset_end += item_size + layout->as.stack.gap;
            } else {
                result.x += layout->as.stack.inset_begin;
                layout->as.stack.inset_begin += item_size + layout->as.stack.gap;
            }
            result.width = item_size;
            break;
        case DIRECTION_VERTICAL:
            if (RL_IS_OPPOSITE(data)) {
                result.y += result.height - layout->as.stack.inset_end - item_size;
                layout->as.stack.inset_end += item_size + layout->as.stack.gap;
            } else {
                result.y += layout->as.stack.inset_begin;
                layout->as.stack.inset_begin += item_size + layout->as.stack.gap;
            }
            result.height = item_size;
            break;
        default:
            DW_UNIMPLEMENTED_MSG("Unknown stack direction `%d`.", layout->as.stack.direction);
        }

        break;
    }
    case LAYOUT_SPACED:
    {
        size_t count = layout->as.spaced.count;
        size_t gap = layout->as.spaced.gap;
        size_t cursor = layout->as.spaced.cursor;

        switch (layout->as.spaced.direction)
        {
        case DIRECTION_HORIZONTAL:
        {
            float cell_size = 1.0 * (result.width - (count - 1) * gap) / count;
            result.x += cursor * (cell_size + gap);
            result.width = ceilf(data * cell_size + (data - 1) * gap);
            break;
        }
        case DIRECTION_VERTICAL:
        {
            float cell_size = 1.0 * (result.height - (count - 1) * gap) / count;
            result.y += cursor * (cell_size + gap);
            result.height = ceilf(data * cell_size + (data - 1) * gap);
            break;
        }
        default:
            DW_UNIMPLEMENTED_MSG("Unknown spaced direction `%d`.", layout->as.stack.direction);
        }

        layout->as.spaced.cursor += data;
        break;
    }
    case LAYOUT_RECTANGLE:
        result = layout->as.rectangle;
        break;
    default:
        DW_UNIMPLEMENTED_MSG("Unknown layout kind `%d`.", layout->kind);
    }

    return result;
}

void _LayoutBegin(RL_Layout layout)
{
    NOB_ASSERT(rl_layout_stack_count < RLLAYOUT_STACK_CAPACITY);
    rl_layout_stack[rl_layout_stack_count] = layout;
    ++rl_layout_stack_count;
}

void LayoutBeginScreen(int padding)
{
    RL_Layout layout = {
        .kind = LAYOUT_SCREEN,
        .parent_bounds = {0},
        .as.screen.padding = padding,
    };
    _LayoutBegin(layout);
}

void LayoutBeginAnchored(int data, int gap)
{
    RL_Layout layout = {
        .kind = LAYOUT_ANCHOR,
        .parent_bounds = LayoutRectangle(data),
        .as.anchored = {
            .inset_left = 0,
            .inset_top = 0,
            .inset_right = 0,
            .inset_bottom = 0,
            .gap = gap,
        },
    };
    _LayoutBegin(layout);
}

void LayoutBeginStack(int data, RL_LayoutDirection direction, int item_size, int gap)
{
    RL_Layout layout = {
        .kind = LAYOUT_STACK,
        .parent_bounds = LayoutRectangle(data),
        .as.stack = {
            .direction = direction,
            .item_size = item_size,
            .gap = gap,
            .inset_begin = 0,
            .inset_end = 0,
        },
    };
    _LayoutBegin(layout);
}

void LayoutBeginSpaced(int data, RL_LayoutDirection direction, int count, int gap)
{
    RL_Layout layout = {
        .kind = LAYOUT_SPACED,
        .parent_bounds = LayoutRectangle(data),
        .as.spaced = {
            .direction = direction,
            .count = count,
            .gap = gap,
            .cursor = 0,
        },
    };
    _LayoutBegin(layout);
}

void LayoutBeginRectangle(int data, Rectangle rectangle)
{
    RL_Layout layout = {
        .kind = LAYOUT_RECTANGLE,
        .parent_bounds = LayoutRectangle(data),
        .as.rectangle = rectangle,
    };
    _LayoutBegin(layout);
}

void LayoutSpacing(int amount)
{
    RL_Layout* layout = &rl_layout_stack[rl_layout_stack_count - 1];
    if (layout->kind == LAYOUT_STACK)
    {
        layout->as.stack.inset_begin += amount;
    }
}

void LayoutEnd()
{
    NOB_ASSERT(rl_layout_stack_count > 0);
    rl_layout_stack_count--;
}

void LayoutGuiLabel(const char* text)
{
    RL_Layout* layout = &rl_layout_stack[rl_layout_stack_count - 1];
    Rectangle bounds = {0};
    if (layout->kind == LAYOUT_STACK && layout->as.stack.direction == DIRECTION_HORIZONTAL) {
        bounds = LayoutRectangle(RL_DEFAULT(GuiGetTextWidth(text) + 2));
    } else if (layout->kind == LAYOUT_STACK && layout->as.stack.direction == DIRECTION_VERTICAL) {
        bounds = LayoutRectangle(RL_DEFAULT(GuiGetStyle(DEFAULT, TEXT_SIZE)));
    } else {
        bounds = LayoutDefault();
    }
    GuiLabel(bounds, text);
}

#define _BUTTON_LABEL_PADDING 7

void LayoutGuiButtonLabel(const char* text)
{
    RL_Layout* layout = &rl_layout_stack[rl_layout_stack_count - 1];
    Rectangle bounds = {0};
    if (layout->kind == LAYOUT_STACK && layout->as.stack.direction == DIRECTION_HORIZONTAL) {
        bounds = LayoutRectangle(RL_DEFAULT(GuiGetTextWidth(text) + 2*_BUTTON_LABEL_PADDING+2));
    } else if (layout->kind == LAYOUT_STACK && layout->as.stack.direction == DIRECTION_VERTICAL) {
        bounds = LayoutRectangle(RL_DEFAULT(GuiGetStyle(DEFAULT, TEXT_SIZE) + 2*_BUTTON_LABEL_PADDING+1));
    } else {
        bounds = LayoutDefault();
    }

    DrawRectangleRounded(bounds, 0.4, 20, GetColor(GuiGetStyle(BUTTON, BASE_COLOR_NORMAL)));
    DrawRectangleRoundedLinesEx(LayoutPaddingAll(bounds, 2), 0.4, 20, 2, GetColor(GuiGetStyle(BUTTON, BORDER_COLOR_NORMAL)));
    GuiLabel(LayoutPaddingSymmetric(bounds, _BUTTON_LABEL_PADDING, 0), text);
}

#endif // RLLAYOUT_IMPLEMENTATION