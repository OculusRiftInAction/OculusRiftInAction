#include "Common.h"

class RiftFramework {
  protected:

  private:
    ovrTexture eyeTextures[2];
    ovrEyeRenderDesc eyeRenderDescs[2];
    glm::mat4 projections[2];
    ovrEyeType currentEye{ ovrEye_Count };
    int frameIndex{ 0 };

  public:
    RiftFramework() {
      ovr_Initialize();
    }

    virtual ~RiftFramework() {
      ovr_Shutdown();
    }

  protected:

    virtual void initRift() {
      ovrHmd_Create(0);
    }

    virtual void initWindow() {
    }

    virtual void initRendering() {
    }

    virtual bool shouldQuit() {
      return true;
    }

    // Called once per frame
    virtual void update() {
    }

    virtual void run() {
      initRift();
      initWindow();
      initRendering();
      while (!shouldQuit()) {
        ++frameIndex;
        update();
        draw();
      }
    }

    virtual void draw() final;
    virtual void postDraw() {};
    virtual void renderScene() = 0;


    virtual void applyEyePoseAndOffset(const glm::mat4 & eyePose, const glm::vec3 & eyeOffset);

    inline ovrEyeType getCurrentEye() const {
      return currentEye;
    }

    const ovrEyeRenderDesc & getEyeRenderDesc(ovrEyeType eye) const {
      return eyeRenderDescs[eye];
    }

    const ovrFovPort & getFov(ovrEyeType eye) const {
      return eyeRenderDescs[eye].Fov;
    }

    const glm::mat4 & getPerspectiveProjection(ovrEyeType eye) const {
      return projections[eye];
    }

    const ovrFovPort & getFov() const {
      return getFov(getCurrentEye());
    }

    const ovrEyeRenderDesc & getEyeRenderDesc() const {
      return getEyeRenderDesc(getCurrentEye());
    }

    const glm::mat4 & getPerspectiveProjection() const {
      return getPerspectiveProjection(getCurrentEye());
    }
};

#if defined(OVR_OS_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#elif defined(OVR_OS_MAC)
#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#elif defined(OVR_OS_LINUX)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#endif

#include <GLFW/glfw3native.h>

using glm::vec3;

static vec3 vv[]{
  vec3(-27135, -47603, -22785),
    vec3(-55528, -45523, -15209),
    vec3(-38354, -41370, 11728),
    vec3(-56988, -19616, 16090),
    vec3(-78636, -35792, 10410),
    vec3(-87153, -27556, -14528),
    vec3(-74327, -44583, -43378),
    vec3(-88396, -21886, -48721),
    vec3(-88274, -60, -15680),
    vec3(-88406, 24904, -48754),
    vec3(-75550, 45763, -43539),
    vec3(-87269, 27411, -14896),
    vec3(-83951, 81, 10877),
    vec3(-78566, 36849, 10558),
    vec3(-56582, 20142, 15304),
    vec3(-55795, 46660, -14716),
    vec3(-28691, 48949, -22661),
    vec3(-28897, 43111, 11246),
    vec3(-31368, 127, 17834),
    vec3(-261, -41971, 12403),
    vec3(-47, -9692, 17947),
    vec3(31178, 247, 17846),
    vec3(28771, 42923, 11417),
    vec3(28737, 48835, -22560),
    vec3(55947, 46663, -14143),
    vec3(56419, 19861, 15420),
    vec3(78393, 37083, 10686),
    vec3(84043, -129, 10895),
    vec3(87262, 27528, -15139),
    vec3(75956, 45582, -43921),
    vec3(88406, 24945, -49351),
    vec3(88346, -24, -15902),
    vec3(88926, -21596, -48943),
    vec3(74759, -44657, -43911),
    vec3(87122, -27692, -14929),
    vec3(78373, -35933, 10698),
    vec3(56520, -19797, 16000),
    vec3(37802, -41586, 11705),
    vec3(55743, -45646, -14518),
    vec3(27368, -47677, -22886),
    vec3(0)
};

class HelloRift : public GlfwApp {
protected:
  glm::mat4               player;

public:
  HelloRift() {
    windowSize = glm::uvec2(800, 600);
    resetPosition();
  }

  ~HelloRift() {
  }

  virtual void resetPosition() {
    static const glm::vec3 EYE = glm::vec3(0, 0, 1.0f);
    static const glm::vec3 LOOKAT = glm::vec3(0, 0, 0);
    player = glm::inverse(glm::lookAt(EYE, LOOKAT, GlUtils::UP));
  }

  
  virtual void createRenderingTarget() {
    createWindow(windowSize, windowPosition);
    glfwSetWindowPos(window, 100, -900);
  }

  void initGl() {
    GlfwApp::initGl();
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (CameraControl::instance().onKey(key, scancode, action, mods)) {
      return;
    }

    GlfwApp::onKey(key, scancode, action, mods);
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);
    gl::Stacks::modelview().top() = glm::inverse(player);
    glm::perspective(60.0f, 800.0f / 600.0f, 0.1f, 100.0f);
  }

  void draw() {
    glClearColor(0, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float scale = 1.0f / 100000.0f;
    glColor3f(1, 1, 1);
    glPointSize(10);
    glBegin(GL_POINTS);
    glVertex3f(0, 0, 0);
    int i = 0;
    while (vec3(0) != vv[i]) {
      vec3 v = vv[i++];
      v /= 100000.0f;
      glVertex3f(v.x, v.y, v.z);
    }
    glEnd();
  }

  
};

RUN_APP(HelloRift);

