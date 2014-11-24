#pragma once

#ifdef HAVE_CEGUI

#pragma warning( disable : 4275 )
#include <CEGUI/CEGUI.h>
#include <CEGUI/MemoryStdAllocator.h>
#pragma warning( default : 4725 )

#define UI_CONTEXT

namespace ui {
  void initWindow(const uvec2 & size);
  bool handleGlfwCharacterEvent(unsigned int codepoint);
  bool handleGlfwKeyboardEvent(int key, int scancode, int action, int mods);
  bool handleGlfwMouseMoveEvent(int x, int y);
  bool handleGlfwMouseScrollEvent(int x, int y);
  bool handleGlfwMouseButtonEvent(int button, int action, int mods);


  class Wrapper {
    uvec2 size;
    CEGUI::FrameWindow * rootWindow;
    FramebufferWrapper fbo;
#ifdef UI_CONTEXT
    GLFWwindow * context{ nullptr };
#endif

  public:
    ~Wrapper();
    void init(const uvec2 & size);
    void update(std::function<void()> f);
    void bindTexture();
    CEGUI::FrameWindow * getWindow();
    void withUiContext(std::function<void()> f);

  };


}


#endif
