#include "Common.h"

#include <opencv2/opencv.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>

#define TEXTURES 6
// #define LATENCY_TEST


template <class T>
class CaptureHandler {
protected:
  volatile bool frameChanged{ false };
  volatile bool stop{ false };
  boost::mutex frameMutex;
  boost::thread captureThread;
  T captureResult;

  CaptureHandler() {
  }

  void startCapture() {
    captureThread = boost::thread(boost::ref(*this));
  }

  const T & getCaptureResult() const {
    return captureResult;
  }

public:
  virtual void operator()() = 0;
};


class DistortionHelper {

protected:
  glm::dvec4 K;

  double getUndistortionScaleForRadiusSquared(double rSq) {
    double distortionScale = K[0]
      + rSq * (K[1] + rSq * (K[2] + rSq * K[3]));
    return distortionScale;
  }

  double getUndistortionScale(const glm::dvec2 & v) {
    double rSq = glm::length2(v);
    return getUndistortionScaleForRadiusSquared(rSq);
  }

  double getUndistortionScaleForRadius(double r) {
    return getUndistortionScaleForRadiusSquared(r * r);
  }

  glm::dvec2 getUndistortedPosition(const glm::dvec2 & v) {
    return v * getUndistortionScale(v);
  }

  bool closeEnough(double a, double b, double epsilon = 1e-5) {
    return abs(a - b) < epsilon;
  }

  double getDistortionScaleForRadius(double rTarget) {
    double max = rTarget * 2;
    double min = 0;
    double distortionScale;
    while (true) {
      double rSource = ((max - min) / 2.0) + min;
      distortionScale = getUndistortionScaleForRadiusSquared(
        rSource * rSource);
      double rResult = distortionScale * rSource;
      if (closeEnough(rResult, rTarget)) {
        break;
      }
      if (rResult < rTarget) {
        min = rSource;
      }
      else {
        max = rSource;
      }
    }
    return 1.0 / distortionScale;
  }

public:
  DistortionHelper(const glm::vec4 & k) {
    K = k;
  }

  gl::GeometryPtr createDistortionMesh(const glm::uvec2 & distortionMeshResolution, float aspect) {
    std::vector<glm::vec4> vertexData;
    vertexData.reserve(
      distortionMeshResolution.x * distortionMeshResolution.y * 2);
    // The texture coordinates are actually from the center of the pixel, so thats what we need to use for the calculation.
    for (size_t y = 0; y < distortionMeshResolution.y; ++y) {
      for (size_t x = 0; x < distortionMeshResolution.x; ++x) {
        // Create a texture coordinate that goes from [0, 1]
        glm::dvec2 texCoord = (glm::dvec2(x, y)
          / glm::dvec2(distortionMeshResolution - glm::uvec2(1)));
        // Create the vertex coordinate in the range [-1, 1]
        glm::dvec2 vertexPos = (texCoord * 2.0) - 1.0;
        vertexPos.y /= aspect;
        double rSquared = glm::length2(vertexPos);
        double distortionScale = getUndistortionScaleForRadiusSquared(rSquared);
        glm::dvec2 result = vertexPos * distortionScale;
        vertexData.push_back(glm::vec4(result, 0, 1));
        vertexData.push_back(glm::vec4(texCoord, 0, 1));
      }
    }

    std::vector<GLuint> indexData;
    for (size_t y = 0; y < distortionMeshResolution.y - 1; ++y) {
      size_t rowStart = y * distortionMeshResolution.x;
      size_t nextRowStart = rowStart + distortionMeshResolution.x;
      for (size_t x = 0; x < distortionMeshResolution.x; ++x) {
        indexData.push_back(nextRowStart + x);
        indexData.push_back(rowStart + x);
      }
      indexData.push_back(UINT_MAX);

    }
    return gl::GeometryPtr(
      new gl::Geometry(vertexData, indexData, indexData.size(),
      gl::Geometry::Flag::HAS_TEXTURE, GL_TRIANGLE_STRIP));
  }
};


class WebcamHandler : public CaptureHandler<cv::Mat> {
private:
  cv::VideoCapture videoCapture{ 0 };
  glm::uvec2 imageSize;

protected:
  WebcamHandler() {
    if (!videoCapture.grab()) {
      FAIL("Failed grab");
    }
    videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
    videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
    int fps = (int)videoCapture.get(CV_CAP_PROP_FPS);
    videoCapture.set(CV_CAP_PROP_FPS, 60);
    // Force minimum focus, and disable autofocus
    //    videoCapture.set(CV_CAP_PROP_FOCUS, 0);
    imageSize = glm::uvec2(videoCapture.get(CV_CAP_PROP_FRAME_WIDTH),
      videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT));
    captureResult = cv::Mat::zeros(imageSize.y, imageSize.x, CV_8UC3);
  }

  virtual void operator()() {
    int framecount = 0;
    long start = Platform::elapsedMillis();
    while (!stop) {
      cv::waitKey(1);
      if (!videoCapture.grab()) {
        FAIL("Failed grab");
      }
      // Wait for OpenGL to consume a frame before fetching a new one
      while (frameChanged) {
        Platform::sleepMillis(10);
      }
      static cv::Mat frame;
      videoCapture.retrieve(frame);
      boost::lock_guard < boost::mutex > lock(frameMutex);
      cv::flip(frame, captureResult, 0);
      frameChanged = true;
    }
  }

  const glm::uvec2 & getCaptureSize() const {
    return imageSize;
  }
};

#define USE_RIFT 1
#ifdef USE_RIFT
#define PARENT_CLASS RiftApp
#else
#define PARENT_CLASS GlfwApp
#endif

class WebcamApp : public WebcamHandler, public PARENT_CLASS
{
protected:
  size_t currentTexture{ 0 };
  gl::Texture2dPtr textures[TEXTURES];

#ifdef LATENCY_TEST
  OVR::Ptr<OVR::LatencyTestDevice> ovrLatencyTester;
  OVR::Util::LatencyTest latencyTest;
  GLFWwindow * latencyWindow;
#endif
  gl::GeometryPtr videoGeometry;
  glm::quat predictionDelta;
  glm::quat riftOrientation;
  float videoFps{ 0 };
  bool strabsimusCorrectionEnabled;
  float webcamLatency{ 0.161f };
  float riftLatency{ 0.028f };
  time_t lastWebcamImage;
  glm::vec4 k{ 5.37f, 3.83f, -4.2f, 25.0f};
  float scale{ 0.69f };

public:

  WebcamApp() : currentTexture(0) {
#ifdef USE_RIFT
    sensorFusion.SetGravityEnabled(true);
    sensorFusion.SetPredictionEnabled(true);
#endif
    startCapture();
  }

#ifndef USE_RIFT
  virtual void createRenderingTarget() {
    createWindow(glm::uvec2(960, 540), glm::ivec2(1920 - 1000, -600));
  }
#endif

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action && GLFW_REPEAT != action) {
      GlfwApp::onKey(key, scancode, action, mods);
      return;
    }
    switch (key) {
#if 0
    case GLFW_KEY_Y:
      k[0] += (mods & GLFW_MOD_SHIFT) ? 0.1 : 0.01;
      videoGeometry = DistortionHelper(k).createDistortionMesh(glm::uvec2(64, 64), glm::aspect(getCaptureSize()));
      return;
    case GLFW_KEY_H:
      k[0] -= (mods & GLFW_MOD_SHIFT) ? 0.1 : 0.01;
      videoGeometry = DistortionHelper(k).createDistortionMesh(glm::uvec2(64, 64), glm::aspect(getCaptureSize()));
      return;
    case GLFW_KEY_U:
      k[1] += (mods & GLFW_MOD_SHIFT) ? 0.1 : 0.01;
      videoGeometry = DistortionHelper(k).createDistortionMesh(glm::uvec2(64, 64), glm::aspect(getCaptureSize()));
      return;
    case GLFW_KEY_J:
      k[1] -= (mods & GLFW_MOD_SHIFT) ? 0.1 : 0.01;
      videoGeometry = DistortionHelper(k).createDistortionMesh(glm::uvec2(64, 64), glm::aspect(getCaptureSize()));
      return;
    case GLFW_KEY_I:
      k[2] += (mods & GLFW_MOD_SHIFT) ? 0.1 : 0.01;
      videoGeometry = DistortionHelper(k).createDistortionMesh(glm::uvec2(64, 64), glm::aspect(getCaptureSize()));
      return;
    case GLFW_KEY_K:
      k[2] -= (mods & GLFW_MOD_SHIFT) ? 0.1 : 0.01;
      videoGeometry = DistortionHelper(k).createDistortionMesh(glm::uvec2(64, 64), glm::aspect(getCaptureSize()));
      return;
    case GLFW_KEY_O:
      k[3] += (mods & GLFW_MOD_SHIFT) ? 0.1 : 0.01;
      videoGeometry = DistortionHelper(k).createDistortionMesh(glm::uvec2(64, 64), glm::aspect(getCaptureSize()));
      return;
    case GLFW_KEY_L:
      k[3] -= (mods & GLFW_MOD_SHIFT) ? 0.1 : 0.01;
      videoGeometry = DistortionHelper(k).createDistortionMesh(glm::uvec2(64, 64), glm::aspect(getCaptureSize()));
      return;
#endif
    case GLFW_KEY_LEFT_BRACKET:
      scale += (mods & GLFW_MOD_SHIFT) ? 0.1 : 0.01;
      return;
    case GLFW_KEY_RIGHT_BRACKET:
      scale -= (mods & GLFW_MOD_SHIFT) ? 0.1 : 0.01;
      return;

#ifdef USE_RIFT
    case GLFW_KEY_P:
      sensorFusion.SetPredictionEnabled(!sensorFusion.IsPredictionEnabled());
      break;
    case GLFW_KEY_UP:
      if (sensorFusion.IsPredictionEnabled()) {
        sensorFusion.SetPrediction(sensorFusion.GetPredictionDelta() + 0.01f, true);
      }
      // videoCapture.set(CV_CAP_PROP_FOCUS, videoCapture.get(CV_CAP_PROP_FOCUS) + 5);
      return;
    case GLFW_KEY_DOWN:
      if (sensorFusion.IsPredictionEnabled()) {
        sensorFusion.SetPrediction(sensorFusion.GetPredictionDelta() - 0.01f, true);
      }
      // videoCapture.set(CV_CAP_PROP_FOCUS, videoCapture.get(CV_CAP_PROP_FOCUS) - 5);
      return;
#endif
    }
    PARENT_CLASS::onKey(key, scancode, action, mods);
  }


  void initGl() {
    PARENT_CLASS::initGl();
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(UINT_MAX);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < TEXTURES; i++) {
      textures[i] = gl::TexturePtr(new gl::Texture2d());
      textures[i]->bind();
      textures[i]->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      textures[i]->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    gl::Texture2d::unbind();
    videoGeometry = DistortionHelper(k).createDistortionMesh(glm::uvec2(64, 64), glm::aspect(getCaptureSize()));
    //videoGeometry = GlUtils::getQuadGeometry(glm::aspect(getCaptureSize()));
  }

  virtual void update() {
    if (frameChanged) {
      boost::lock_guard<boost::mutex> lock(frameMutex);
      currentTexture = (currentTexture + 1) % TEXTURES;
      textures[currentTexture]->bind();
      textures[currentTexture]->image2d(getCaptureSize(), getCaptureResult().data, 0, GL_BGR);
      frameChanged = false;
      lastWebcamImage = Platform::elapsedMillis();
    }

#ifdef USE_RIFT
    riftOrientation = Rift::fromOvr(sensorFusion.GetPredictedOrientation(riftLatency));

#ifdef LATENCY_TEST
    latencyTest.ProcessInputs();
#endif
    float currentWebcamLatency = (float)(Platform::elapsedMillis() - lastWebcamImage) / 1000.0f;
    glm::quat webcamRiftOrientation = Rift::fromOvr(sensorFusion.GetPredictedOrientation(webcamLatency + riftLatency - currentWebcamLatency));
    predictionDelta = webcamRiftOrientation * glm::inverse(Rift::fromOvr(sensorFusion.GetOrientation()));

    gl::Stacks::modelview().top() = glm::lookAt(glm::vec3(0, 0, 2.0f), glm::vec3(), GlUtils::Y_AXIS);
#endif
  }

#ifdef USE_RIFT
  virtual void renderScene() {
#else
  virtual void draw() {
#endif
    glEnable(GL_DEPTH_TEST);
    gl::clearColor(glm::vec3(0.05f, 0.05f, 0.05f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.with_push([&]{
      mv.preMultiply(glm::mat4_cast(predictionDelta));
      mv.scale(scale);

      gl::ProgramPtr videoRenderProgram = GlUtils::getProgram(
        Resource::SHADERS_TEXTURED_VS,
        Resource::SHADERS_TEXTURED_FS);
      videoRenderProgram->use();
      textures[currentTexture]->bind();
      GlUtils::renderGeometry(videoGeometry, videoRenderProgram);

      gl::Texture2d::unbind();
      gl::VertexArray::unbind();
      gl::Program::clear();
    });

    mv.with_push([&]{
      mv.translate(glm::vec2(0.9, -0.6));
      mv.rotate(riftOrientation);
      mv.scale(0.4f);
      GlUtils::renderArtificialHorizon(0.4f);
    });


    //glDisable(GL_DEPTH_TEST);
    //pr.push().top() = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f) * eyeArgs.projectionOffset;
    //mv.push().identity();
    //glm::vec2 cursor(-0.4, 0.4);
    //std::string message = Platform::format(
    //  "OpenGL FPS: %0.2f\n"
    //  "Vidcap FPS: %0.2f\n"
    //  "Prediction: %0.2fms\n",
    //  fps, videoFps,
    //  webcamLatency * 1000.0f);
    // GlUtils::renderString(message, cursor, 12.0f);
    //glEnable(GL_DEPTH_TEST);

  }
};

RUN_OVR_APP(WebcamApp);

