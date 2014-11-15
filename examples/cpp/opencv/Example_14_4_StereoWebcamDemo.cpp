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
      Sleep(10);
    }
    if (!videoCapture.read(frame.image)) {
      FAIL("Could not open get first frame from webcam %i", which);
    }
    float aspectRatio = (float)frame.image.cols / (float)frame.image.rows;
    captureThread = std::thread(&WebcamHandler::captureLoop, this);

    // Snooze for 200 ms to get past multithreading issues in OpenCV
    Sleep(200);
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

  gl::Texture2dPtr texture[2];
  gl::GeometryPtr videoGeometry[2];
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

    for (int i = 0; i < 2; i++) {
      texture[i] = GlUtils::initTexture();
      videoGeometry[i] = GlUtils::getQuadGeometry(captureHandler[i].startCapture(hmd, CAMERA_FOR_EYE[i]));
    }
  }

  virtual void update() {
    for (int i = 0; i < 2; i++) {
      if (captureHandler[i].get(captureData[i])) {
        texture[i]->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
            captureData[i].image.cols, captureData[i].image.rows,
            0, GL_BGR, GL_UNSIGNED_BYTE,
            captureData[i].image.data);
        texture[i]->unbind();
      }
    }
  }

  virtual void renderScene() {
    glClear(GL_DEPTH_BUFFER_BIT);
    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    
    mv.with_push([&]{
      glm::quat eyePose = Rift::fromOvr(getEyePose().Orientation);
      glm::quat webcamPose = Rift::fromOvr(captureData[getCurrentEye()].pose.Orientation);
      glm::mat4 webcamDelta = glm::mat4_cast(glm::inverse(eyePose) * webcamPose);

      mv.identity();
      mv.preMultiply(webcamDelta);

      mv.translate(glm::vec3(0, 0, -2.75));
      texture[getCurrentEye()]->bind();
      GlUtils::renderGeometry(videoGeometry[getCurrentEye()]);
      texture[getCurrentEye()]->unbind();
    });
  }
};

RUN_OVR_APP(WebcamApp);
