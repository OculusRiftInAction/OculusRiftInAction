#pragma once

#ifdef HAVE_CEGUI

#pragma warning( disable : 4275 )
#include <CEGUI/CEGUI.h>
#include <CEGUI/MemoryStdAllocator.h>
#pragma warning( default : 4725 )


namespace ui {
  void initWindow(const uvec2 & size);
  bool handleGlfwCharacterEvent(unsigned int codepoint);
  bool handleGlfwKeyboardEvent(int key, int scancode, int action, int mods);
  bool handleGlfwMouseMoveEvent(int x, int y);
  bool handleGlfwMouseScrollEvent(int x, int y);
  bool handleGlfwMouseButtonEvent(int button, int action, int mods);


  class Wrapper {
    ProgramPtr program;
    ShapeWrapperPtr shape;
    FramebufferWrapper fbo;
    uvec2 size;
    GLFWwindow * context{ nullptr };
    CEGUI::FrameWindow * rootWindow;


  public:
    ~Wrapper();
    void init(const uvec2 & size, std::function<void()> f);
    void update();
    void render();
    CEGUI::FrameWindow * getWindow();
  };


}


#endif
