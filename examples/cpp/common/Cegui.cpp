#include "Common.h"

#ifdef HAVE_CEGUI
#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>

namespace ui {


  /// Helper Macro
#define mapKey(a) case GLFW_KEY_##a: return Key::a;
#define mapKey2(a, b) case GLFW_KEY_##a: return Key::b;
#define mapMouse(a, b) case GLFW_MOUSE_BUTTON_##a: return b;

  CEGUI::MouseButton TranslateMouseButton(int glfwButton)
  {
    using namespace CEGUI;
    switch (glfwButton)
    {
        mapMouse(LEFT, LeftButton)
      mapMouse(RIGHT, RightButton)
      mapMouse(MIDDLE, MiddleButton)
    default:
      return CEGUI::MouseButton::NoButton;
    }
  }

  CEGUI::Key::Scan TranslateKey(int glfwKey) {
    using namespace CEGUI;
    switch (glfwKey) {
      mapKey2(SPACE, Space)
        mapKey2(APOSTROPHE, Apostrophe)
        mapKey2(COMMA, Comma)
        mapKey2(MINUS, Minus)
        mapKey2(PERIOD, Period)
        mapKey2(SLASH, Slash)
        mapKey2(0, Zero)
        mapKey2(1, One)
        mapKey2(2, Two)
        mapKey2(3, Three)
        mapKey2(4, Four)
        mapKey2(5, Five)
        mapKey2(6, Six)
        mapKey2(7, Seven)
        mapKey2(8, Eight)
        mapKey2(9, Nine)
        mapKey2(SEMICOLON, Semicolon)
        mapKey2(EQUAL, Equals)
        mapKey(A)
        mapKey(B)
        mapKey(C)
        mapKey(D)
        mapKey(E)
        mapKey(F)
        mapKey(G)
        mapKey(H)
        mapKey(I)
        mapKey(J)
        mapKey(K)
        mapKey(L)
        mapKey(M)
        mapKey(N)
        mapKey(O)
        mapKey(P)
        mapKey(Q)
        mapKey(R)
        mapKey(S)
        mapKey(T)
        mapKey(U)
        mapKey(V)
        mapKey(W)
        mapKey(X)
        mapKey(Y)
        mapKey(Z)

        mapKey2(LEFT_BRACKET, LeftBracket)
        mapKey2(BACKSLASH, Backslash)
        mapKey2(RIGHT_BRACKET, RightBracket)
        mapKey2(GRAVE_ACCENT, Grave)
        mapKey2(ESCAPE, Escape)
        mapKey2(ENTER, Return)
        mapKey2(TAB, Tab)
        mapKey2(BACKSPACE, Backspace)
        mapKey2(INSERT, Insert)
#undef DELETE
        mapKey2(DELETE, Delete)

        mapKey2(RIGHT, ArrowRight)
        mapKey2(LEFT, ArrowLeft)
        mapKey2(DOWN, ArrowDown)
        mapKey2(UP, ArrowUp)
        mapKey2(PAGE_UP, PageUp)
        mapKey2(PAGE_DOWN, PageDown)
        mapKey2(HOME, Home)
        mapKey2(END, End)
        // mapKey2(CAPS_LOCK, CapsLock)
        mapKey2(SCROLL_LOCK, ScrollLock)
        mapKey2(NUM_LOCK, NumLock)
        // mapKey2(PRINT_SCREEN, PrintScreen)
        mapKey2(PAUSE, Pause)
        mapKey(F1)
        mapKey(F2)
        mapKey(F3)
        mapKey(F4)
        mapKey(F5)
        mapKey(F6)
        mapKey(F7)
        mapKey(F8)
        mapKey(F9)
        mapKey(F10)
        mapKey(F11)
        mapKey(F12)
        mapKey(F13)
        mapKey(F14)
        mapKey(F15)
        mapKey2(KP_0, Numpad0)
        mapKey2(KP_1, Numpad1)
        mapKey2(KP_2, Numpad2)
        mapKey2(KP_3, Numpad3)
        mapKey2(KP_4, Numpad4)
        mapKey2(KP_5, Numpad5)
        mapKey2(KP_6, Numpad6)
        mapKey2(KP_7, Numpad7)
        mapKey2(KP_8, Numpad8)
        mapKey2(KP_9, Numpad9)
        mapKey2(KP_DECIMAL, Decimal)
        mapKey2(KP_DIVIDE, Divide)
        mapKey2(KP_MULTIPLY, Multiply)
        mapKey2(KP_SUBTRACT, Subtract)
        mapKey2(KP_ADD, Add)
        mapKey2(KP_ENTER, NumpadEnter)
        mapKey2(KP_EQUAL, NumpadEquals)
        mapKey2(LEFT_SHIFT, LeftShift)
        mapKey2(LEFT_CONTROL, LeftControl)
        mapKey2(LEFT_ALT, LeftAlt)
        mapKey2(LEFT_SUPER, LeftWindows)
        mapKey2(RIGHT_SHIFT, RightShift)
        mapKey2(RIGHT_CONTROL, RightControl)
        mapKey2(RIGHT_ALT, RightAlt)
        mapKey2(RIGHT_SUPER, RightWindows)
        mapKey2(MENU, AppMenu)
    default:
      return CEGUI::Key::Unknown;
    }
  }

  void initWindow(const uvec2 & size) {
    using namespace CEGUI;
    static CEGUI::OpenGL3Renderer & myRenderer =
      CEGUI::OpenGL3Renderer::create();
    myRenderer.enableExtraStateSettings (true);
    
    myRenderer.setDisplaySize(CEGUI::Sizef(size.x, size.y));
    CEGUI::System::create(myRenderer);
//    CEGUI::Logger::getSingleton().setLogFilename("/dev/cegui.log");
//    CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::LoggingLevel::Insane);
    //  OpenGL3Renderer::bootstrapSystem();

    {
      DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>(CEGUI::System::getSingleton().getResourceProvider());
      rp->setResourceGroupDirectory("schemes", PROJECT_DIR "/resources/cegui/schemes/");
      rp->setResourceGroupDirectory("imagesets", PROJECT_DIR "/resources/cegui/imagesets/");
      rp->setResourceGroupDirectory("fonts", PROJECT_DIR "/resources/cegui/fonts/");
      rp->setResourceGroupDirectory("layouts", PROJECT_DIR "/resources/cegui/layouts/");
      rp->setResourceGroupDirectory("looknfeels", PROJECT_DIR "/resources/cegui/looknfeel/");
      rp->setResourceGroupDirectory("lua_scripts", PROJECT_DIR "/resources/cegui/lua_scripts/");
      rp->setResourceGroupDirectory("resources", "/");
    }

    // set the default resource groups to be used
    ImageManager::setImagesetDefaultResourceGroup("imagesets");
    Font::setDefaultResourceGroup("fonts");
    Scheme::setDefaultResourceGroup("schemes");
    WidgetLookManager::setDefaultResourceGroup("looknfeels");
    WindowManager::setDefaultResourceGroup("layouts");
    // ScriptModule::setDefaultResourceGroup("lua_scripts");
    // AnimationManager::setDefaultResourceGroup("animations");

    SchemeManager::getSingleton().createFromFile("TaharezLook.scheme");
    FontManager::getSingleton().createFromFile("Bitwise-24.font");

    System::getSingleton().getDefaultGUIContext().setDefaultFont("Bitwise-24");
    System::getSingleton().getDefaultGUIContext().getMouseCursor().setDefaultImage("TaharezLook/MouseArrow");
  }

  CEGUI::GUIContext & getContext() {
    return CEGUI::System::getSingleton().getDefaultGUIContext();
  }

  bool handleGlfwCharacterEvent(unsigned int codepoint) {
    return getContext().injectChar(codepoint);
  }

  bool handleGlfwKeyboardEvent(int key, int scancode, int action, int mods) {
    switch (action) {
    case GLFW_PRESS:
      return getContext().injectKeyDown(TranslateKey(key));

    case GLFW_RELEASE:
      return getContext().injectKeyUp(TranslateKey(key));
    }
    return false;
  }

  bool handleGlfwMouseMoveEvent(int x, int y) {
    getContext().injectMousePosition(x, y);
    return true;
  }

  bool handleGlfwMouseScrollEvent(int x, int y) {
    getContext().injectMouseWheelChange(y);
    return true;
  }

  bool handleGlfwMouseButtonEvent(int button, int action, int mods) {
    switch (action) {
    case GLFW_PRESS:
      return getContext().injectMouseButtonDown(TranslateMouseButton(button));

    case GLFW_RELEASE:
      return getContext().injectMouseButtonUp(TranslateMouseButton(button));
    }
    return true;
  }

    //case SDL_MOUSEWHEEL:
    //System::getSingleton().getDefaultGUIContext().injectMouseWheelChange(event.wheel.y);
    //return true;
    //case SDL_KEYDOWN:
    //return true;
}
#endif
