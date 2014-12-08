#include "Common.h"

#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>

int CAMERA_FOR_EYE[2] = { 2, 1 };

struct CaptureData {
  ovrPosef pose;
  cv::Mat image;
};

class WebcamHandler {

private:

  bool hasFrame{ false };
  bool stopped{ false };
  cv::VideoCapture videoCapture;
  std::thread captureThread;
  std::mutex mutex;
  CaptureData frame;
  ovrHmd hmd;

public:

  WebcamHandler() {
  }

  // Spawn capture thread and return webcam aspect ratio (width over height)
  float startCapture(ovrHmd & hmdRef, int which) {
    hmd = hmdRef;
    videoCapture.open(which);
    if (!videoCapture.isOpened()) {
      FAIL("Could not open video source from webcam %i", which);
    }
    for (int i = 0; i < 10 && !videoCapture.read(frame.image); i++) {
      Platform::sleepMillis(10);
    }
    if (!videoCapture.read(frame.image)) {
      FAIL("Could not open get first frame from webcam %i", which);
    }
    float aspectRatio = (float)frame.image.cols / (float)frame.image.rows;
    captureThread = std::thread(&WebcamHandler::captureLoop, this);

    // Snooze for 200 ms to get past multithreading issues in OpenCV
    Platform::sleepMillis(200);
    return aspectRatio;
  }

  void stopCapture() {
    stopped = true;
    captureThread.join();
    videoCapture.release();
  }

  void set(const CaptureData & newFrame) {
    std::lock_guard<std::mutex> guard(mutex);
    frame = newFrame;
    hasFrame = true;
  }

  bool get(CaptureData & out) {
    if (!hasFrame) {
      return false;
    }
    std::lock_guard<std::mutex> guard(mutex);
    out = frame;
    hasFrame = false;
    return true;
  }

  void captureLoop() {
    CaptureData captured;
    while (!stopped) {
      float captureTime = ovr_GetTimeInSeconds();
      ovrTrackingState tracking = ovrHmd_GetTrackingState(hmd, captureTime);
      captured.pose = tracking.HeadPose.ThePose;

      videoCapture.read(captured.image);
      cv::flip(captured.image.clone(), captured.image, 0);
      set(captured);
    }
  }
};

class WebcamApp : public RiftApp {

protected:

  ProgramPtr program;
  TexturePtr texture[2];
  ShapeWrapperPtr videoGeometry[2];
  WebcamHandler captureHandler[2];
  CaptureData captureData[2];

public:

  WebcamApp() {
  }

  virtual ~WebcamApp() {
    for (int i = 0; i < 2; i++) {
      captureHandler[i].stopCapture();
    }
  }

  void initGl() {
    RiftApp::initGl();
    using namespace oglplus;

    program = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);

    for (int i = 0; i < 2; i++) {
      texture[i] = TexturePtr(new Texture());
      Context::Bound(TextureTarget::_2D, *texture[i])
        .MagFilter(TextureMagFilter::Linear)
        .MinFilter(TextureMinFilter::Linear);
      program = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
      float aspect = captureHandler[i].startCapture(hmd, CAMERA_FOR_EYE[i]);
      videoGeometry[i] = oria::loadPlane(program, aspect);
    }
  }

  virtual void update() {
    for (int i = 0; i < 2; i++) {
      if (captureHandler[i].get(captureData[i])) {
        using namespace oglplus;
        Context::Bound(TextureTarget::_2D, *texture[i])
          .Image2D(0, PixelDataInternalFormat::RGBA8,
          captureData[i].image.cols, captureData[i].image.rows, 0,
          PixelDataFormat::BGR, PixelDataType::UnsignedByte,
          captureData[i].image.data);
      }
    }
  }

  virtual void renderScene() {
    using namespace oglplus;
    glClear(GL_DEPTH_BUFFER_BIT);
    oria::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    MatrixStack & mv = Stacks::modelview();

    mv.withPush([&] {
      glm::quat eyePose = ovr::toGlm(getEyePose().Orientation);
      glm::quat webcamPose = ovr::toGlm(captureData[getCurrentEye()].pose.Orientation);
      glm::mat4 webcamDelta = glm::mat4_cast(glm::inverse(eyePose) * webcamPose);

      mv.identity();
      mv.preMultiply(webcamDelta);

      mv.translate(glm::vec3(0, 0, -2.75));
      texture[getCurrentEye()]->Bind(TextureTarget::_2D); 
      oria::renderGeometry(videoGeometry[getCurrentEye()], program);
    });
    oglplus::DefaultTexture().Bind(TextureTarget::_2D);
  }
};

RUN_OVR_APP(WebcamApp);
