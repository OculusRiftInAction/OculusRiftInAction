#include "Common.h"

#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>

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

  WebcamHandler(ovrHmd & hmd) : hmd(hmd) {
  }

  // Spawn capture thread and return webcam aspect ratio (width over height)
  float startCapture() {
    videoCapture.open(1);
    if (!videoCapture.isOpened()
      || !videoCapture.read(frame.image)) {
      FAIL("Could not open video source to capture first frame");
    }
    float aspectRatio = (float)frame.image.cols / (float)frame.image.rows;
    captureThread = std::thread(&WebcamHandler::captureLoop, this);
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

  gl::Texture2dPtr texture;
  gl::GeometryPtr videoGeometry;
  WebcamHandler captureHandler;
  CaptureData captureData;

public:

  WebcamApp() : captureHandler(hmd) {
  }

  virtual ~WebcamApp() {
    captureHandler.stopCapture();
  }

  void initGl() {
    RiftApp::initGl();

    texture = GlUtils::initTexture();
    float aspectRatio = captureHandler.startCapture();
    videoGeometry = GlUtils::getQuadGeometry(aspectRatio);
  }

  virtual void update() {
    if (captureHandler.get(captureData)) {
      texture->bind();
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
          captureData.image.cols, captureData.image.rows,
          0, GL_BGR, GL_UNSIGNED_BYTE,
          captureData.image.data);
      texture->unbind();
    }
  }

  virtual void renderScene() {
    glClear(GL_DEPTH_BUFFER_BIT);
    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    
    mv.with_push([&]{
      glm::quat eyePose = Rift::fromOvr(getEyePose().Orientation);
      glm::quat webcamPose = Rift::fromOvr(captureData.pose.Orientation);
      glm::mat4 webcamDelta = glm::mat4_cast(glm::inverse(eyePose) * webcamPose);

      mv.identity();
      mv.preMultiply(webcamDelta);

      mv.translate(glm::vec3(0, 0, -2.75));
      texture->bind();
      GlUtils::renderGeometry(videoGeometry);
      texture->unbind();
    });
  }
};

RUN_OVR_APP(WebcamApp);
