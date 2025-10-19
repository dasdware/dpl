#ifndef __RL_DIALOG_H
#define __RL_DIALOG_H

#define RL_DIALOG_PATH_CAPACITY 1024
#define RL_DIALOG_FILENAME_CAPACITY 261

#include <arena.h>
#include <raylib.h>

typedef enum
{
    RLFD_UI_NONE,
    RLFD_UI_PARENT_DIRECTORY,
    RLFD_UI_SELECTED_DIRECTORY,
    RLFD_UI_FILES,
    RLFD_UI_SELECTED_FILE,
    RLFD_UI_OK,
    RLFD_UI_CANCEL,
    RLFD_UI_MAX,
} RL_FileDialogUI;

typedef enum
{
    ITEM_UNKNOWN,
    ITEM_DIRECTORY,
    ITEM_PARENT_DIRECTORY,
    ITEM_FILE,
} RL_FileListItemKind;

typedef struct
{
    RL_FileListItemKind kind;
    char name[RL_DIALOG_FILENAME_CAPACITY];
} RL_FileListItem;

typedef struct
{
    RL_FileListItem *items;
    size_t count;
    size_t capacity;
} RL_FileList;

typedef struct
{
    Arena memory;

    const char *title;
    const char *filter;

    bool opened;
    RL_FileDialogUI active_ui;

    RL_FileList files;
    Vector2 files_scroll;
    int files_index;
    bool selected_file_exists;

    double key_down_state;
    double key_up_state;

    char *selected_directory;
    char *editing_directory;
    char *selected_filename;
    char *editing_filename;
} RL_FileDialog;

void GuiFileDialogInit(RL_FileDialog *dialog, const char *title, const char *filename, const char *filter);
void GuiFileDialogFree(RL_FileDialog *dialog);

void GuiFileDialogOpen(RL_FileDialog *dialog);
void GuiFileDialogCheck(RL_FileDialog *dialog);

int GuiFileDialog(RL_FileDialog *dialog);

const char* GuiFileDialogFileName(RL_FileDialog* dialog);

#endif // __RL_DIALOG_H

#ifdef RL_DIALOG_IMPLEMENTATION

#include <nob.h>
#include <raygui.h>
#include <rllayout.h>

#define RL_DIALOG_KEY_INITIAL_COOLDOWN 0.5f
#define RL_DIALOG_KEY_COOLDOWN 0.075f

bool _GuiFileDialogKeyCooldownEx(KeyboardKey key, float initialDelay, float delay, float* state)
{
    if (IsKeyDown(key)) {
        double now = GetTime();
        if (*state <= 0.f) {
            *state = now + (initialDelay - delay);
            return true;
        }
        if (now - *state >= delay) {
            *state = now;
            return true;
        }
    } else {
        *state = -1.f;
    }
    return false;
}

bool _GuiFileDialogKeyCooldown(KeyboardKey key, float* state) {
    return _GuiFileDialogKeyCooldownEx(key, RL_DIALOG_KEY_INITIAL_COOLDOWN, RL_DIALOG_KEY_COOLDOWN, state);
}

void _GuiFileDialogCheckSelectedFile(RL_FileDialog *dialog)
{
    dialog->selected_file_exists =
        FileExists(TextFormat("%s" NOB_PATH_DELIM_STR "%s", dialog->selected_directory, dialog->selected_filename));
}

void _GuiFileDialogReadFiles(RL_FileDialog *dialog)
{
    dialog->files.count = 0;
    dialog->files_index = -1;

    DIR *dir = opendir(dialog->selected_directory);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0)
        {
            continue;
        }

        RL_FileListItem item;
        const char *fullname = TextFormat("%s" NOB_PATH_DELIM_STR "%s", dialog->selected_directory, entry->d_name);
        if (DirectoryExists(fullname))
        {
            if (strcmp(entry->d_name, "..") == 0)
            {
                item.kind = ITEM_PARENT_DIRECTORY;
            }
            else
            {
                item.kind = ITEM_DIRECTORY;
            }
        }
        else if (FileExists(fullname))
        {
            if (dialog->filter != NULL && !IsFileExtension(entry->d_name, dialog->filter))
            {
                continue;
            }

            item.kind = ITEM_FILE;
        }
        else
        {
            item.kind = ITEM_UNKNOWN;
        }

        if (dialog->files_index == -1 && strcmp(dialog->selected_filename, entry->d_name) == 0)
        {
            dialog->files_index = dialog->files.count;
        }

        strcpy_s(item.name, 261, entry->d_name);
        nob_da_append(&dialog->files, item);
    }

    if (dialog->files_index == -1)
    {
        dialog->files_index = 0;
    }

    closedir(dir);

    _GuiFileDialogCheckSelectedFile(dialog);

    dialog->files_scroll.y = 0.f;
}

void _GuiFileDialogGotoParentDirectory(RL_FileDialog *dialog)
{
    strcpy_s(dialog->selected_directory, 1024, GetPrevDirectoryPath(dialog->selected_directory));
    strcpy_s(dialog->editing_directory, 1024, dialog->selected_directory);
    _GuiFileDialogReadFiles(dialog);
}

void _GuiFileDialogGotoDirectory(RL_FileDialog *dialog)
{
    RL_FileListItem item = dialog->files.items[dialog->files_index];
    snprintf(dialog->selected_directory, 1024, "%s" NOB_PATH_DELIM_STR "%s", dialog->selected_directory, item.name);
    strcpy_s(dialog->editing_directory, 1024, dialog->selected_directory);
    _GuiFileDialogReadFiles(dialog);
}

void _GuiFileDialogUpdateFileNames(RL_FileDialog *dialog)
{
    RL_FileListItem item = dialog->files.items[dialog->files_index];
    if (item.kind == ITEM_FILE)
    {
        strcpy_s(dialog->selected_filename, 1024, item.name);
        strcpy_s(dialog->editing_filename, 1024, item.name);
        _GuiFileDialogCheckSelectedFile(dialog);
    }
}

int _GuiFileDialogButton(RL_FileDialog *dialog, RL_FileDialogUI id, Rectangle bounds, const char *text)
{
    if (id == dialog->active_ui)
    {
        GuiSetState(STATE_FOCUSED);
    }

    int result = GuiButton(bounds, text);

    if (id == dialog->active_ui)
    {
        GuiSetState(STATE_NORMAL);

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
        {
            result = 1;
        }
    }

    return result;
}

void _GuiFileDialogCheckSelectedItemBounds(RL_FileDialog *dialog, Rectangle view, int listItemHeight) {
    float item_top = dialog->files_scroll.y + dialog->files_index * listItemHeight;
    if (item_top < 0) {
        dialog->files_scroll.y -= item_top;
    } else {
        float item_bottom = dialog->files_scroll.y + (dialog->files_index + 1) * listItemHeight - 1;
        if (item_bottom > view.height) {
            dialog->files_scroll.y -= item_bottom - view.height;
        }
    }
}

void GuiFileDialogInit(RL_FileDialog *dialog, const char *title, const char *filename, const char *filter)
{
    dialog->title = title;
    dialog->filter = filter;
    dialog->active_ui = RLFD_UI_FILES;

    dialog->opened = false;

    char real_filename[RL_DIALOG_PATH_CAPACITY];
    nob_realpath(filename, real_filename, RL_DIALOG_PATH_CAPACITY);

    dialog->selected_directory = arena_alloc(&dialog->memory, RL_DIALOG_PATH_CAPACITY);
    dialog->selected_filename = arena_alloc(&dialog->memory, RL_DIALOG_PATH_CAPACITY);
    dialog->editing_directory = arena_alloc(&dialog->memory, RL_DIALOG_PATH_CAPACITY);
    dialog->editing_filename = arena_alloc(&dialog->memory, RL_DIALOG_PATH_CAPACITY);

    if (filename && DirectoryExists(filename))
    {
        strcpy_s(dialog->selected_directory, 1024, real_filename);
        strcpy_s(dialog->selected_filename, 1024, "");
    }
    else if (filename && FileExists(real_filename))
    {
        strcpy_s(dialog->selected_directory, 1024, GetDirectoryPath(real_filename));
        strcpy_s(dialog->selected_filename, 1024, GetFileName(real_filename));
    }
    else
    {
        strcpy_s(dialog->selected_directory, 1024, GetWorkingDirectory());
        strcpy_s(dialog->selected_filename, 1024, "");
    }

    strcpy_s(dialog->editing_directory, 1024, dialog->selected_directory);
    strcpy_s(dialog->editing_filename, 1024, dialog->selected_filename);

    dialog->key_up_state = -1.f;
    dialog->key_down_state = -1.f;

    _GuiFileDialogReadFiles(dialog);
}

void GuiFileDialogFree(RL_FileDialog *dialog)
{
    nob_da_free(dialog->files);
    arena_free(&dialog->memory);
}

void GuiFileDialogOpen(RL_FileDialog *dialog)
{
    dialog->active_ui = RLFD_UI_FILES;
    dialog->opened = true;
}

void GuiFileDialogCheck(RL_FileDialog *dialog)
{
    if (dialog->opened)
    {
        GuiLock();
    }
}

int GuiFileDialog(RL_FileDialog *dialog)
{
    int result = 0;

    if (dialog->opened)
    {
        GuiUnlock();

        if (IsKeyPressed(KEY_TAB))
        {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
            {
                dialog->active_ui--;
                if (dialog->active_ui <= RLFD_UI_NONE)
                {
                    dialog->active_ui = RLFD_UI_MAX - 1;
                }
            }
            else
            {
                dialog->active_ui++;
                if (dialog->active_ui >= RLFD_UI_MAX)
                {
                    dialog->active_ui = RLFD_UI_NONE + 1;
                }
            }
        }

        LayoutBeginScreen(0);
        {
            DrawRectangleRec(LayoutDefault(), CLITERAL(Color) {
                0, 0, 0, 192
            });

            Rectangle dialog_bounds = LayoutCenter(LayoutDefault(), 800, 600);

            if (GuiWindowBox(dialog_bounds, dialog->title) || (!GuiIsLocked() && IsKeyPressed(KEY_ESCAPE)))
            {
                dialog->opened = false;
            }

            LayoutBeginRectangle(RLD_DEFAULT, LayoutPadding(dialog_bounds, 12, 35, 12, 12));
            {

                LayoutBeginStack(RLD_DEFAULT, DIRECTION_VERTICAL, 30, 10);
                {
                    LayoutBeginStack(RLD_DEFAULT, DIRECTION_HORIZONTAL, 30, 10);
                    {
                        if (_GuiFileDialogButton(dialog, RLFD_UI_PARENT_DIRECTORY, LayoutDefault(), "#03#"))
                        {
                            _GuiFileDialogGotoParentDirectory(dialog);
                        }

                        if (GuiTextBox(LayoutRemaining(), dialog->editing_directory, 1024, dialog->active_ui == RLFD_UI_SELECTED_DIRECTORY))
                        {
                            if (dialog->active_ui == RLFD_UI_SELECTED_DIRECTORY)
                            {
                                if (DirectoryExists(dialog->editing_directory))
                                {
                                    strcpy_s(dialog->selected_directory, 1024, dialog->editing_directory);
                                    _GuiFileDialogReadFiles(dialog);
                                }
                            }
                            else
                            {
                                dialog->active_ui = RLFD_UI_SELECTED_DIRECTORY;
                            }
                        }
                    }
                    LayoutEnd();

                    LayoutBeginStack(RLD_OPPOSITE, DIRECTION_HORIZONTAL, 100, 10);
                    {
                        if (_GuiFileDialogButton(dialog, RLFD_UI_CANCEL, LayoutOpposite(), "Cancel"))
                        {
                            dialog->opened = false;
                        }

                        if (!dialog->selected_file_exists)
                            GuiDisable();

                        if (_GuiFileDialogButton(dialog, RLFD_UI_OK, LayoutOpposite(), "Open"))
                        {
                            result = 1;
                            dialog->opened = false;
                        }

                        GuiEnable();
                    }
                    LayoutEnd();

                    LayoutBeginStack(RLD_OPPOSITE, DIRECTION_HORIZONTAL, 100, 10);
                    {
                        GuiLabel(LayoutDefault(), "File");

                        if (GuiTextBox(LayoutRemaining(), dialog->editing_filename, 1024, dialog->active_ui == RLFD_UI_SELECTED_FILE))
                        {
                            if (dialog->active_ui == RLFD_UI_SELECTED_FILE)
                            {
                                dialog->active_ui = RLFD_UI_NONE;
                                // TODO: Reread directory contents
                            }
                            else
                            {
                                dialog->active_ui = RLFD_UI_SELECTED_FILE;
                            }
                        }
                    }
                    LayoutEnd();

                    if (RLFD_UI_FILES == dialog->active_ui)
                    {
                        GuiSetState(STATE_FOCUSED);
                    }

                    int listItemTextSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
                    int listItemPadding = 4;
                    int listItemHeight = listItemTextSize + 2 * listItemPadding;
                    int listItemIconSize = 16;
                    int listItemIconVOffset = (listItemHeight - listItemIconSize) / 2;

                    Rectangle list_bounds = LayoutRemaining();
                    Rectangle files_bounds = {0, 0, list_bounds.width - 2 * GuiGetStyle(DEFAULT, BORDER_WIDTH), listItemHeight * dialog->files.count};
                    if (files_bounds.height > list_bounds.height - 2 * GuiGetStyle(DEFAULT, BORDER_WIDTH)) {
                        files_bounds.width -= GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH);
                    }

                    Rectangle view;
                    GuiScrollPanel(list_bounds, NULL, files_bounds, &dialog->files_scroll, &view);

                    BeginScissorMode(view.x, view.y, view.width, view.height);

                    for (size_t i = 0; i < dialog->files.count; ++i)
                    {
                        RL_FileListItem item = dialog->files.items[i];
                        Rectangle file_bounds = {view.x, view.y + dialog->files_scroll.y + i * listItemHeight, view.width, listItemHeight - 1};
                        if (CheckCollisionRecs(file_bounds, view))
                        {
                            bool hovering = CheckCollisionPointRec(GetMousePosition(), file_bounds)
                                            && CheckCollisionPointRec(GetMousePosition(), view);
                            if ((int)i == dialog->files_index)
                            {
                                if (GuiGetState() == STATE_FOCUSED)
                                {
                                    DrawRectangleRec(file_bounds, GetColor(GuiGetStyle(TEXTBOX, BASE_COLOR_PRESSED)));
                                    DrawRectangleLinesEx(file_bounds, GuiGetStyle(TEXTBOX, BORDER_WIDTH), GetColor(GuiGetStyle(TEXTBOX, BORDER_COLOR_FOCUSED)));
                                }
                                else
                                {
                                    DrawRectangleRec(file_bounds, GetColor(GuiGetStyle(TEXTBOX, BASE_COLOR_DISABLED)));
                                    DrawRectangleLinesEx(file_bounds, GuiGetStyle(TEXTBOX, BORDER_WIDTH), GetColor(GuiGetStyle(TEXTBOX, BORDER_COLOR_NORMAL)));
                                }
                            }
                            else if (hovering)
                            {
                                DrawRectangleRec(file_bounds, GetColor(GuiGetStyle(TEXTBOX, BASE_COLOR_DISABLED)));
                            }

                            if (hovering)
                            {
                                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                                {
                                    dialog->active_ui = RLFD_UI_FILES;
                                    dialog->files_index = (int)i;
                                    _GuiFileDialogUpdateFileNames(dialog);
                                    _GuiFileDialogCheckSelectedItemBounds(dialog, view, listItemHeight);
                                }

                                if (GetGestureDetected() == GESTURE_DOUBLETAP)
                                {
                                    if (item.kind == ITEM_PARENT_DIRECTORY)
                                    {
                                        _GuiFileDialogGotoParentDirectory(dialog);
                                    }
                                    else if (item.kind == ITEM_DIRECTORY)
                                    {
                                        _GuiFileDialogGotoDirectory(dialog);
                                    }
                                    else if (item.kind == ITEM_FILE && dialog->selected_file_exists)
                                    {
                                        result = 1;
                                        dialog->opened = false;
                                    }
                                }
                            }

                            if ((int)i == dialog->files_index && IsKeyPressed(KEY_ENTER))
                            {
                                if (item.kind == ITEM_PARENT_DIRECTORY)
                                {
                                    _GuiFileDialogGotoParentDirectory(dialog);
                                }
                                else if (item.kind == ITEM_DIRECTORY)
                                {
                                    _GuiFileDialogGotoDirectory(dialog);
                                }
                                else if (item.kind == ITEM_FILE && dialog->selected_file_exists)
                                {
                                    result = 1;
                                    dialog->opened = false;
                                }
                            }

                            int icon_id;
                            switch (item.kind)
                            {
                            case ITEM_FILE:
                                icon_id = ICON_FILE;
                                break;
                            case ITEM_DIRECTORY:
                                icon_id = ICON_FOLDER;
                                break;
                            case ITEM_PARENT_DIRECTORY:
                                icon_id = ICON_FOLDER_OPEN;
                                break;
                            case ITEM_UNKNOWN:
                            default:
                                icon_id = ICON_HELP;
                            }

                            Color foreground;
                            if (GuiGetState() == STATE_FOCUSED)
                            {
                                foreground = GetColor(GuiGetStyle(LABEL, TEXT_COLOR_FOCUSED));
                            }
                            else
                            {
                                foreground = GetColor(GuiGetStyle(LABEL, TEXT_COLOR_NORMAL));
                            }

                            GuiDrawIcon(icon_id, file_bounds.x + listItemPadding, file_bounds.y + listItemIconVOffset, 1, foreground);
                            DrawText(item.name, file_bounds.x + 2 * listItemPadding + listItemIconSize, file_bounds.y + listItemPadding, listItemTextSize, foreground);
                        }
                    }
                    EndScissorMode();

                    if (RLFD_UI_FILES == dialog->active_ui)
                    {
                        if (_GuiFileDialogKeyCooldown(KEY_UP, &dialog->key_up_state) && dialog->files_index > 0)
                        {
                            dialog->files_index--;
                            _GuiFileDialogUpdateFileNames(dialog);
                            _GuiFileDialogCheckSelectedItemBounds(dialog, view, listItemHeight);
                        }
                        else if (_GuiFileDialogKeyCooldown(KEY_DOWN, &dialog->key_down_state) && dialog->files_index < (int)dialog->files.count - 1)
                        {
                            dialog->files_index++;
                            _GuiFileDialogUpdateFileNames(dialog);
                            _GuiFileDialogCheckSelectedItemBounds(dialog, view, listItemHeight);
                        }
                        else if (IsKeyPressed(KEY_BACKSPACE))
                        {
                            _GuiFileDialogGotoParentDirectory(dialog);
                        }

                        GuiSetState(STATE_NORMAL);
                    }
                }
                LayoutEnd();
            }
            LayoutEnd();
        }
        LayoutEnd();
    }

    return result;
}

const char* GuiFileDialogFileName(RL_FileDialog* dialog) {
    static char fullname[1024];
    snprintf(fullname, 1024, "%s" NOB_PATH_DELIM_STR "%s", dialog->selected_directory, dialog->selected_filename);
    return fullname;
}

#endif