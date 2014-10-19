#include "Common.h"

#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>

#define CAMERA_PARAMS_FILE "camera.xml"
#define CAMERA_WIDTH 1280
#define CAMERA_HEIGHT 720
#define CAMERA_HFOV_DEGREES 70.42f
#define IMAGE_DISTANCE 10.0f
#define CAMERA_DEVICE 1
#define CAMERA_LATENCY 0.040

template <class T>
class CaptureHandler {
private:
  typedef std::lock_guard<std::mutex> lock_guard;

  std::thread captureThread;
  std::mutex mutex;

  bool changed{ false };
  bool stop{ false };

  float firstCapture{ -1 };
  int captures{ -1 };

  T result;

protected:

  bool isStopped() {
    return stop;
  }

  void setResult(const T & newResult) {
    if (0 == ++captures) {
      firstCapture = Platform::elapsedSeconds();
    }
    lock_guard guard(mutex);
    result = newResult;
    changed = true;
  }

public:

  float getCapturesPerSecond() {
    if (-1 == captures) {
      return 0;
    }
    float elapsed = Platform::elapsedSeconds() - firstCapture;
    return (float)captures / elapsed;
  }

  void startCapture() {
    stop = false;
    captureThread = std::thread(&CaptureHandler::captureLoop, this);
  }

  void stopCapture() {
    stop = true;
    captureThread.join();
  }

  bool getResult(T & outResult) {
    if (!changed) {
      return false;
    }
    lock_guard guard(mutex);
    changed = false;
    outResult = result;
    result = T();
    return true;
  }

  virtual void captureLoop() = 0;
};

struct CaptureData {
  ovrPosef pose;
  cv::Mat image;
};

class WebcamCaptureHandler : public CaptureHandler<CaptureData> {
private:
  cv::VideoCapture videoCapture;
  ovrHmd hmd;

public:

  WebcamCaptureHandler(ovrHmd hmd) : hmd(hmd) {
    videoCapture.open(CAMERA_DEVICE);
    if (!videoCapture.isOpened()) {
      FAIL("Could not open video source");
    }

    videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
    videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);
    videoCapture.set(CV_CAP_PROP_FPS, 60);
  }
  
  virtual void captureLoop() {
    while (!isStopped()) {
      CaptureData captured;
      captured.pose = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds() - CAMERA_LATENCY).HeadPose.ThePose;

      if (!videoCapture.grab()) {
        FAIL("Failed video capture");
      }

      if (!videoCapture.retrieve(captured.image)) {
        FAIL("Failed video capture");
      }

      cv::flip(captured.image.clone(), captured.image, 0);
      setResult(captured);
    }
  }
};

class WebcamApp : public RiftApp
{
protected:
  WebcamCaptureHandler captureHandler;
  CaptureData captureData;

  gl::Texture2dPtr texture;
  gl::GeometryPtr videoGeometry;
  gl::ProgramPtr videoRenderProgram;

public:

  WebcamApp() : captureHandler(hmd) {
    captureHandler.startCapture();
  }

  virtual ~WebcamApp() {
    captureHandler.stopCapture();
  }

  void initGl() {
    RiftApp::initGl();
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(UINT_MAX);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    videoRenderProgram = GlUtils::getProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);

    texture = gl::TexturePtr(new gl::Texture2d());
    texture->bind();
    texture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    texture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl::Texture2d::unbind();

    float halfFov = DEGREES_TO_RADIANS * CAMERA_HFOV_DEGREES / 2.0f;
    float scale = tan(halfFov) * IMAGE_DISTANCE;

    videoGeometry = GlUtils::getQuadGeometry((float)CAMERA_WIDTH / (float)CAMERA_HEIGHT, scale * 2.0f);

  }

  virtual void update() {
    if (captureHandler.getResult(captureData)) {
      texture->bind();
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
        captureData.image.cols, captureData.image.rows, 0, GL_BGR, GL_UNSIGNED_BYTE,
        captureData.image.data);
      gl::Texture2d::unbind();
    }
  }

  virtual void renderScene() {
    glClear(GL_DEPTH_BUFFER_BIT);
    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    gl::MatrixStack & mv = gl::Stacks::modelview();

    mv.with_push([&]{
      mv.identity();

      glm::mat4 eyePose = Rift::fromOvr(getEyePose());
      glm::mat4 webcamPose = Rift::fromOvr(captureData.pose);
      glm::mat4 webcamDelta = webcamPose * glm::inverse(eyePose);
      glm::vec3 rotated = glm::mat3(webcamDelta) * glm::vec3(0, 0, -1);
      glm::mat4 lookat = glm::lookAt(glm::vec3(0), rotated, glm::vec3(0, 1, 0));

      mv.preMultiply(glm::inverse(lookat));
      mv.translate(glm::vec3(0, 0, -IMAGE_DISTANCE));

      texture->bind();
      GlUtils::renderGeometry(videoGeometry, videoRenderProgram);
      gl::Texture2d::unbind();
    });

    std::string message = Platform::format(
      "OpenGL FPS: %0.2f\n"
      "Vidcap FPS: %0.2f\n",
      fps, captureHandler.getCapturesPerSecond());
    GlfwApp::renderStringAt(message, glm::vec2(-0.5f, 0.5f));
  }
};

RUN_OVR_APP(WebcamApp);

