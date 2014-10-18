#include "Common.h"

#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>

#define TEXTURES 6
// #define LATENCY_TEST

#define CAMERA_PARAMS_FILE "camera.xml"

template <class T>
class CaptureHandler {
protected:
  volatile bool frameChanged{ false };
  volatile bool stop{ false };
  std::mutex frameMutex;
  std::thread captureThread;
  T captureResult;

  CaptureHandler() {
  }

  void startCapture() {
    captureThread = std::thread(&CaptureHandler::operator(), this);
  }

  const T & getCaptureResult() const {
    return captureResult;
  }

public:
  virtual void operator()() = 0;
};


class WebcamHandler : public CaptureHandler<cv::Mat> {
private:
  cv::VideoCapture videoCapture{ 1 };
  cv::Size imageSize{ 1280, 720 };
  cv::Mat cameraMatrix;
  cv::Mat distCoeffs;
  bool hasCalibration{ false };

protected:
  double lastFrameTimes[2];

  WebcamHandler() {
    cv::Mat view, rview, map1, map2;

    
    cv::FileStorage fs(CAMERA_PARAMS_FILE, cv::FileStorage::READ); // Read the settings
    if (fs.isOpened()) {
      fs["Camera_Matrix"] >> cameraMatrix;
      fs["Distortion_Coefficients"] >> distCoeffs;
      hasCalibration = true;
    }


    if (!videoCapture.grab()) {
      FAIL("Failed grab");
    }
    videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, imageSize.width);
    videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, imageSize.height);
    int fps = (int)videoCapture.get(CV_CAP_PROP_FPS);
    videoCapture.set(CV_CAP_PROP_FPS, 60);
    captureResult = cv::Mat::zeros(imageSize, CV_8UC3);
  }

  virtual void operator()() {
    int framecount = 0;
    long start = Platform::elapsedMillis();
    while (!stop) {
      cv::waitKey(1);
      if (!videoCapture.grab()) {
        FAIL("Failed grab");
      }
      lastFrameTimes[0] = lastFrameTimes[1];
      lastFrameTimes[1] = ovr_GetTimeInSeconds();
      // Wait for OpenGL to consume a frame before fetching a new one
      while (frameChanged) {
        Platform::sleepMillis(10);
      }
      static cv::Mat frame;
      videoCapture.retrieve(frame);
      if (hasCalibration) {
        cv::Mat temp = frame.clone();
        undistort(temp, frame, cameraMatrix, distCoeffs);
      }
      std::lock_guard < std::mutex > lock(frameMutex);
      cv::flip(frame, captureResult, 0);
      frameChanged = true;
    }
  }

  glm::uvec2 getCaptureSize() const {
    return glm::uvec2(imageSize.width, imageSize.height);
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

  gl::GeometryPtr videoGeometry;
  glm::quat predictionDelta;
  float videoFps{ 0 };
  bool strabsimusCorrectionEnabled;
  float webcamLatency{ 0.361f };
  glm::quat webcamOrientation;
  time_t lastWebcamImage;
  float scale{ 0.69f };

public:

  WebcamApp() : currentTexture(0) {
    startCapture();
  }

#ifndef USE_RIFT
  virtual void createRenderingTarget() {
    createWindow(glm::uvec2(960, 540), glm::ivec2(1920 - 1000, -600));
    gl::Stacks::projection().top() = glm::perspective(PI / 3.0f, (float)960 / (float)540, 0.1f, 10000.0f);
    gl::Stacks::modelview().top() = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0), glm::vec3(0, 1, 0));
  }
#endif

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action && GLFW_REPEAT != action) {
      GlfwApp::onKey(key, scancode, action, mods);
      return;
    }
    switch (key) {
    case GLFW_KEY_LEFT_BRACKET:
      scale += (mods & GLFW_MOD_SHIFT) ? 0.1f : 0.01f;
      return;
    case GLFW_KEY_RIGHT_BRACKET:
      scale -= (mods & GLFW_MOD_SHIFT) ? 0.1f : 0.01f;
      return;

#ifdef USE_RIFT
    case GLFW_KEY_P:
      break;
    case GLFW_KEY_UP:
      // videoCapture.set(CV_CAP_PROP_FOCUS, videoCapture.get(CV_CAP_PROP_FOCUS) + 5);
      return;
    case GLFW_KEY_DOWN:
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


    videoGeometry = GlUtils::getQuadGeometry(1280.0f / 720.0f); 
    //DistortionHelper(k).createDistortionMesh(glm::uvec2(64, 64), glm::aspect(getCaptureSize()));
    //videoGeometry = GlUtils::getQuadGeometry(glm::aspect(getCaptureSize()));
  }

  virtual void update() {
    if (frameChanged) {
      std::lock_guard<std::mutex> lock(frameMutex);
      currentTexture = (currentTexture + 1) % TEXTURES;
      textures[currentTexture]->bind();
      textures[currentTexture]->image2d(getCaptureSize(), getCaptureResult().data, 0, GL_BGR);
      webcamLatency = lastFrameTimes[1] - lastFrameTimes[0];
      frameChanged = false;
      lastWebcamImage = Platform::elapsedMillis();
    }

#ifdef USE_RIFT
    double currentWebcamLatency = (float)(Platform::elapsedMillis() - lastWebcamImage) / 1000.0f;
    double webcamPredictionTime = ovr_GetTimeInSeconds() - currentWebcamLatency;
    ovrTrackingState webcamState = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds() + webcamPredictionTime);
    webcamOrientation = Rift::fromOvr(webcamState.HeadPose.ThePose.Orientation);
    predictionDelta = webcamOrientation; //* glm::inverse(Rift::fromOvr(webcamState.Recorded.Pose.Orientation));

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
    glDisable(GL_DEPTH_TEST);
    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.with_push([&]{
      mv.preMultiply(glm::mat4_cast(predictionDelta));
      mv.scale(scale);

      gl::ProgramPtr videoRenderProgram = GlUtils::getProgram(
        Resource::SHADERS_TEXTURED_VS,
        Resource::SHADERS_TEXTURED_FS);
      videoRenderProgram->use();
      textures[currentTexture]->bind();

      if (false) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(3.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      }

      GlUtils::renderGeometry(videoGeometry, videoRenderProgram);

      gl::Texture2d::unbind();
      gl::VertexArray::unbind();
      gl::Program::clear();
    });

    //mv.with_push([&]{
    //  mv.translate(glm::vec2(0.9, -0.6));
    //  mv.rotate(riftOrientation);
    //  mv.scale(0.4f);
    //  GlUtils::renderArtificialHorizon(0.4f);
    //});


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

