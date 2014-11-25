#include "Common.h"

#ifdef HAVE_CEGUI

#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>
#include <CEGUI/CompositeResourceProvider.h>

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

  using namespace CEGUI;
  class MyResourceProvider : public CEGUI::DefaultResourceProvider {
    std::map<CEGUI::String, Resource> invMap;

    Resource getResourceForPath(const CEGUI::String & filename) {
      if (!invMap.count(filename)) {
        return NO_RESOURCE;
      }
      return invMap[filename];
    }

  public:
    MyResourceProvider() {
      for (int i = 0; i != Resource::NO_RESOURCE; ++i) {
        Resource res = static_cast<Resource>(i);
        invMap[Resources::getResourceMnemonic(res).c_str()] = res;
      }
      setResourceGroupDirectory("schemes", "cegui/schemes/");
      setResourceGroupDirectory("imagesets", "cegui/imagesets/");
      setResourceGroupDirectory("fonts", "cegui/fonts/");
      setResourceGroupDirectory("layouts", "cegui/layouts/");
      setResourceGroupDirectory("looknfeels", "cegui/looknfeel/");
      setResourceGroupDirectory("lua_scripts", "cegui/lua_scripts/");
      setResourceGroupDirectory("resources", "");
    }

    virtual void loadRawDataContainer(const CEGUI::String& filename, CEGUI::RawDataContainer& output, const CEGUI::String& resourceGroup) {
      const CEGUI::String path(getFinalFilename(filename, resourceGroup));
      Resource res = getResourceForPath(path);
      assert(res != NO_RESOURCE);
      size_t size = Resources::getResourceSize(res);
      uint8_t* const buffer = static_cast<uint8_t*>(RawDataContainer::Allocator::allocateBytes(size));
      Resources::getResourceData(res, buffer);
      output.setData(buffer);
      output.setSize(size);
    }

    virtual void unloadRawDataContainer(CEGUI::RawDataContainer& container)  {
      container.release();
    }

    virtual size_t getResourceGroupFileNames(std::vector<CEGUI::String>& out_vec, const CEGUI::String& file_pattern, const CEGUI::String& resource_group) {
      const CEGUI::String path(getFinalFilename(file_pattern, resource_group));

      Resource res = getResourceForPath(path);
      if (res == NO_RESOURCE) {
        return 0;
      }
      out_vec.push_back(file_pattern);
      return 1;
    }
  };

  template <typename T>
  T * ceguiCreate() {
    void * buffer = DefaultResourceProvider::Allocator::allocateBytes(sizeof(T));
    return new (buffer)T();
  }

  void initWindow(const uvec2 & size) {
    using namespace CEGUI;
    static CEGUI::OpenGL3Renderer & myRenderer =
      CEGUI::OpenGL3Renderer::create();
    myRenderer.enableExtraStateSettings(true);
    myRenderer.setDisplaySize(CEGUI::Sizef(size.x, size.y));

    CEGUI::System::create(myRenderer, ceguiCreate<MyResourceProvider>());
    //    CEGUI::Logger::getSingleton().setLogFilename("/dev/cegui.log");
    //    CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::LoggingLevel::Insane);

    // set the default resource groups to be used
    ImageManager::setImagesetDefaultResourceGroup("imagesets");
    Font::setDefaultResourceGroup("fonts");
    Scheme::setDefaultResourceGroup("schemes");
    WidgetLookManager::setDefaultResourceGroup("looknfeels");
    WindowManager::setDefaultResourceGroup("layouts");
    AnimationManager::setDefaultResourceGroup("animations");
    SchemeManager::getSingleton().createFromFile("TaharezLook.scheme");
    FontManager::getSingleton().createFromFile("Bitwise-24.font");
    System::getSingleton().getDefaultGUIContext().setDefaultFont("Bitwise-24");
    System::getSingleton().getDefaultGUIContext().getMouseCursor().setDefaultImage("TaharezLook/MouseArrow");
    System::getSingleton().setDefaultCustomRenderedStringParser(new BasicRenderedStringParser());
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

  Wrapper::~Wrapper() {
#ifdef UI_CONTEXT
    if (context) {
      glfwDestroyWindow(context);
      context = nullptr;
    }
#endif
  }

  CEGUI::FrameWindow * Wrapper::getWindow() {
    return rootWindow;
  }

  void Wrapper::init(const uvec2 & size) {
    using namespace oglplus;
    this->size = size;

    glfwWindowHint(GLFW_VISIBLE, 0);
    context = glfwCreateWindow(100, 100, "", nullptr, glfwGetCurrentContext());
    glfwWindowHint(GLFW_VISIBLE, 1);

    withUiContext([&]{
      fbo.init(size);
      fbo.Bound([&]{
        ui::initWindow(size);
        CEGUI::WindowManager & wmgr = CEGUI::WindowManager::getSingleton();
        rootWindow = dynamic_cast<CEGUI::FrameWindow *>(wmgr.createWindow("TaharezLook/FrameWindow", "root"));
        rootWindow->setFrameEnabled(false);
        rootWindow->setTitleBarEnabled(false);
        rootWindow->setAlpha(1.0f);
        rootWindow->setCloseButtonEnabled(false);
        rootWindow->setSize(CEGUI::USize(cegui_absdim(size.x), cegui_absdim(size.y)));
      });
    });
  }

  void Wrapper::update(std::function<void()> f) {
    using namespace oglplus;
    withUiContext([&]{
      f();
      fbo.Bound([&]{
        Context::Viewport(0, 0, size.x, size.y);
        DefaultTexture().Bind(oglplus::Texture::Target::_2D);
        NoProgram().Bind();
        oglplus::Texture::Active(0);
        Context::ClearColor(0, 0, 0, 1);
        Context::Clear().ColorBuffer().DepthBuffer();
        Context::Enable(Capability::Blend);
        Context::Enable(Capability::ScissorTest);
        Context::Enable(Capability::DepthTest);
        Context::Enable(Capability::CullFace);
        CEGUI::System::getSingleton().renderAllGUIContexts();
      });
    });
  }

  void Wrapper::bindTexture() {
    fbo.color->Bind(oglplus::Texture::Target::_2D);
  }

  void Wrapper::withUiContext(std::function<void()> f) {
#ifdef UI_CONTEXT
    withContext(context, [&]{
#endif
      f();
#ifdef UI_CONTEXT
    });
#endif
  }
}

#endif
