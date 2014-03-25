#include "Common.h"
#include "steamvr.h"

typedef gl::Texture<GL_TEXTURE_2D, GL_RG32F> DispTex;
typedef DispTex::Ptr DispTexPtr;

glm::mat4 toGlm(const vr::HmdMatrix44_t & m) {
  glm::mat4 result;
  memcpy(&result[0], m.m, sizeof(float)* 16);
  result = glm::transpose(result);
  return result;
}

glm::mat4 toGlm(const vr::HmdMatrix34_t & m) {
  glm::mat4 result;
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 4; ++col) {
      result[col][row] = m.m[row][col];
    }
  }
  return result;
}

class HelloRift : public GlfwApp {
protected:
  vr::IHmd *  pHMD; 
  gl::FrameBufferWrapper        frameBuffer;
  gl::GeometryPtr               quadGeometry;
  glm::mat4                     camera;

  struct EyeArg {
    glm::uvec2                  viewportLocation;
    glm::uvec2                  viewportSize;
    glm::mat4                   projection;
    glm::mat4                   eyeMatrix;
    DispTexPtr                  displacementTexture;
  } eyeArgs[2];


public:
  HelloRift() {
    vr::HmdError error;
    pHMD = vr::VR_Init(&error);
    if (!pHMD) {
      FAIL("Unable to initialize OVR Device Manager");
    }

    if (!pHMD->GetWindowBounds(&windowPosition.x, &windowPosition.y, &windowSize.x, &windowSize.y)) {
      FAIL("Unable to get window bounds");
    }

    camera = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.4f));

    for (vr::Hmd_Eye eye = vr::Eye_Left; eye < 2; eye = static_cast<vr::Hmd_Eye>(eye + 1)) {
      EyeArg & ea = eyeArgs[eye];
      glm::uvec2 & vppos = ea.viewportLocation;
      glm::uvec2 & vpsize = ea.viewportSize;
      pHMD->GetEyeOutputViewport(eye, vr::API_OpenGL, &vppos.x, &vppos.y, &vpsize.x, &vpsize.y);
      vppos.y = 0;

      vr::HmdMatrix44_t matrix = pHMD->GetProjectionMatrix(eye, 0.01f, 1000.0f, vr::API_OpenGL);
      ea.projection = toGlm(matrix);
    }

    update();
    SAY("Done with ctor");
  }

  ~HelloRift() {
    vr::VR_Shutdown();
  }

  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);

    createWindow(windowSize, windowPosition);
    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
    glfwSwapInterval(1);
  }

#define RESOLUTION 128

  void initGl() {
    GlfwApp::initGl();
    uint32_t fbwidth, fbheight;
    pHMD->GetRecommendedRenderTargetSize(&fbwidth, &fbheight);
    frameBuffer.init(glm::uvec2(fbwidth, fbheight));
    quadGeometry = GlUtils::getQuadGeometry();
    gl::Program::clear();

    for (vr::Hmd_Eye eye = vr::Eye_Left; eye < 2; eye = static_cast<vr::Hmd_Eye>(eye + 1)) {
      EyeArg & ea = eyeArgs[eye];
      glm::uvec2 size;
      float buffer[RESOLUTION * RESOLUTION * 2];
      for (int y = 0; y < RESOLUTION; ++y) {
        float v = (float)y / (float)RESOLUTION;
        size_t rowOffset = RESOLUTION * 2 * y;
        for (int x = 0; x < RESOLUTION; ++x) {
          size_t offset = x * 2 + rowOffset;
          float u = (float)x / (float)RESOLUTION;
          vr::DistortionCoordinates_t d = pHMD->ComputeDistortion(eye, u, v);
          buffer[offset] = d.rfGreen[0];
          buffer[offset + 1] = d.rfGreen[1];
        }
      }

      DispTexPtr & texture = ea.displacementTexture;
      texture = DispTexPtr(new DispTex());
      texture->bind();
      texture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      texture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      texture->parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      texture->parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      texture->image2d(glm::uvec2(RESOLUTION, RESOLUTION), buffer, 0, GL_RG, GL_FLOAT);
//      GlUtils::getImageAsTexture(ea.displacementTexture, IMAGES_SHOULDER_CAT_PNG, size);
      DispTex::unbind();
    }
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      GlfwApp::onKey(key, scancode, action, mods);
      return;
    }

    switch (key) {
    case GLFW_KEY_R:
      pHMD->ZeroTracker();
      return;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
  }

  virtual void update() {
    vr::HmdMatrix44_t matrices[2];
    vr::HmdTrackingResult result;
    pHMD->GetViewMatrix(0.03, &matrices[0], &matrices[1], &result);
    for (int i = 0; i < 2; ++i) {
      eyeArgs[i].eyeMatrix = toGlm(matrices[i]);
    }
    // This corrects the issues with the parallax.
    // std::swap(eyeArgs[0].eyeMatrix, eyeArgs[1].eyeMatrix);
  }

  void draw() {
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    static gl::GeometryPtr geometry = GlUtils::getQuadGeometry(1.0f);

    for (vr::Hmd_Eye eye = vr::Eye_Left; eye < 2; eye = static_cast<vr::Hmd_Eye>(eye + 1)) {
      EyeArg & ea = eyeArgs[eye];

      frameBuffer.activate();
      renderScene(ea);
      frameBuffer.deactivate();
      glDisable(GL_DEPTH_TEST);

      gl::viewport(ea.viewportLocation, ea.viewportSize);
      gl::ProgramPtr program = GlUtils::getProgram(
        SHADERS_TEXTURED_VS, 
        SHADERS_RIFTWARP_FS);
      program->use();
      program->setUniform1i("Scene", 0);
      program->setUniform1i("OffsetMap", 1);
      glActiveTexture(GL_TEXTURE1);
      ea.displacementTexture->bind();
      glActiveTexture(GL_TEXTURE0);
      frameBuffer.color->bind();

      geometry->bindVertexArray();
      geometry->draw();
      gl::Program::clear();
      gl::VertexArray::unbind();
      gl::Texture2d::unbind();
      DispTex::unbind();
    }
  }

  void renderScene(const EyeArg & ea) {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    gl::MatrixStack & pm = gl::Stacks::projection();
    pm.push().top() = ea.projection;

    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.push().top() = ea.eyeMatrix * glm::inverse(camera);

    // Render scene contents
    {
      // Draw a cube about the width of the IPD, at 40 
      // centimeters from the viewer
      mv.push().scale(0.06);
      GlUtils::drawColorCube();
      mv.pop();
    }

    mv.pop();
    pm.pop();
  }
};

RUN_APP(HelloRift);

