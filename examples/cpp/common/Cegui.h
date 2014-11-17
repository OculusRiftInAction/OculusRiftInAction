#pragma once

#ifdef HAVE_CEGUI

#include <CEGUI/CEGUI.h>

namespace ui {
  void initWindow(const uvec2 & size);
  bool handleGlfwCharacterEvent(unsigned int codepoint);
  bool handleGlfwKeyboardEvent(int key, int scancode, int action, int mods);
  bool handleGlfwMouseMoveEvent(int x, int y);
  bool handleGlfwMouseScrollEvent(int x, int y);
  bool handleGlfwMouseButtonEvent(int button, int action, int mods);
}

#endif
