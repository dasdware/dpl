#ifndef __RL_GUIEXT_H
#define __RL_GUIEXT_H

#include <stdbool.h>

bool GuiIsKeyPressed(int key);

#endif // __RL_GUIEXT_H

#ifdef RL_GUIEXT_IMPLEMENTATION

#include <raygui.h>

bool GuiIsKeyPressed(int key) {
    return !GuiIsLocked() && IsKeyPressed(key);
}

#endif // RL_GUIEXT_IMPLEMENTATION