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

  TexturePtr texture;
  ProgramPtr program;
  ShapeWrapperPtr videoGeometry;
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
    using namespace oglplus;
    texture = TexturePtr(new Texture());
    Context::Bound(TextureTarget::_2D, *texture)
      .MagFilter(TextureMagFilter::Linear)
      .MinFilter(TextureMinFilter::Linear);
    program = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
    float aspectRatio = captureHandler.startCapture();
    videoGeometry = oria::loadPlane(program, aspectRatio);
  }

  virtual void update() {
    if (captureHandler.get(captureData)) {
      using namespace oglplus;
      Context::Bound(TextureTarget::_2D, *texture)
        .Image2D(0, PixelDataInternalFormat::RGBA8,
        captureData.image.cols, captureData.image.rows, 0,
        PixelDataFormat::BGR, PixelDataType::UnsignedByte,
        captureData.image.data);
    }
  }

  virtual void renderScene() {
    glClear(GL_DEPTH_BUFFER_BIT);
    oria::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    MatrixStack & mv = Stacks::modelview();

    mv.withPush([&] {
      glm::quat eyePose = ovr::toGlm(getEyePose().Orientation);
      glm::quat webcamPose = ovr::toGlm(captureData.pose.Orientation);
      glm::mat4 webcamDelta = glm::mat4_cast(glm::inverse(eyePose) * webcamPose);

      mv.identity();
      mv.preMultiply(webcamDelta);

      mv.translate(glm::vec3(0, 0, -2));
      using namespace oglplus;
      texture->Bind(TextureTarget::_2D);
      oria::renderGeometry(videoGeometry, program);
      oglplus::DefaultTexture().Bind(TextureTarget::_2D);
    });
  }
};

RUN_OVR_APP(WebcamApp);
