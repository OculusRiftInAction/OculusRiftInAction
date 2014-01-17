#include "Common.h"

using namespace OVR;
using namespace gl;
#include <glm/gtc/noise.hpp>
#include <opencv2/opencv.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>

#define TEXTURES 6

class Example05 : public RiftApp {
protected:
  size_t currentTexture;
  gl::Texture2dPtr textures[TEXTURES];

  // OpenCV stuff
  // The video capture device.  Grabs on to the first web 
  // camera it finds.
  cv::VideoCapture videoCapture;
  // The most recent still from the camera, ready for 
  // copying to an OpenGL texture
  cv::Mat currentFrame;
  // Set to true by the video read thread whenever a 
  // new frame is grabbed and then set back to false 
  // whenever OpenGL consumes that frame.  
  volatile bool frameChanged;
  // A lock to ensure only one thread is accessing 
  // currentFrame / frameChanged at a time
  boost::mutex frameMutex;
  // A background thread for capturing stills from the camera
  boost::thread videoCaptureThread;
  // The size of the captured images
  glm::ivec2 imageSize;
  gl::GeometryPtr videoGeometry;

  glm::quat predictionDelta;
  glm::quat riftOrientation;
 
public:

  Example05() : videoCapture(0), currentTexture(0), frameChanged(false) {
    videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
    videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
    // Force minimum focus, and disable autofocus
    videoCapture.set(CV_CAP_PROP_FOCUS, 0);
    imageSize = glm::ivec2(
      videoCapture.get(CV_CAP_PROP_FRAME_WIDTH), 
      videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT));

    sensorFusion.SetGravityEnabled(true);
    sensorFusion.SetPredictionEnabled(true);
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action && GLFW_REPEAT != action) {
      GlfwApp::onKey(key, scancode, action, mods);
      return;
    }
    switch (key) {
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
    }

    GlfwApp::onKey(key, scancode, action, mods);
  }


  void initGl() {
    RiftApp::initGl();
    for (int i = 0; i < TEXTURES; i++) {
      textures[i] = gl::TexturePtr(new Texture2d());
      textures[i]->bind();
      textures[i]->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      textures[i]->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    gl::Texture2d::unbind();
    videoCaptureThread = boost::thread(boost::ref(*this));
    videoGeometry = GlUtils::getQuadGeometry(glm::aspect(imageSize));
  }

  void operator ()() {
    int nextTexture = (currentTexture + 1) % TEXTURES;
    cv::Mat frame;

    int framecount = 0;
    long start = Platform::elapsedMillis();
    while (!glfwWindowShouldClose(window)) {
      cv::waitKey(1);
      if (!videoCapture.grab()) {
        FAIL("Failed grab");
      }

      // Wait for OpenGL to consume a frame before fetching a new one
      while (frameChanged) {
        Platform::sleepMillis(10);
      }
      videoCapture.retrieve(frame);
      {
        // boost::unique_lock<boost::mutex> lock(frameMutex);
        boost::lock_guard<boost::mutex> lock(frameMutex);
        cv::flip(frame, currentFrame, 0);
        frameChanged = true;
      }
      long now = Platform::elapsedMillis();
      ++framecount;
      if ((now - start) >= 2000) {
        float elapsed = (now - start) / 1000.f;
        float fps = (float)framecount / elapsed;
        SAY("Video Capture FPS: %0.2f\n", fps);
        start = now;
        framecount = 0;
      }
    }
  }

  virtual void update() {
    if (frameChanged) {
      boost::lock_guard<boost::mutex> lock(frameMutex);
      currentTexture = (currentTexture + 1) % TEXTURES;
      textures[currentTexture]->bind();
      textures[currentTexture]->image2d(imageSize, currentFrame.data, 0, GL_BGR);
      frameChanged = false;
    }
    riftOrientation = Rift::fromOvr(sensorFusion.GetPredictedOrientation());
    predictionDelta = riftOrientation * glm::inverse(Rift::fromOvr(sensorFusion.GetOrientation()));
    gl::Stacks::modelview().top() = glm::lookAt(glm::vec3(0, 0, 2.0f), glm::vec3(), GlUtils::Y_AXIS);
  }

  virtual void renderScene(const PerEyeArgs & eyeArgs) {
    gl::Stacks::projection().top() = eyeArgs.getProjection();

    glEnable(GL_DEPTH_TEST);
    clearColor(glm::vec3(0.05f, 0.05f, 0.05f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();
    // Skybox
    {
      mv.push().preMultiply(glm::mat4_cast(riftOrientation)).preMultiply(eyeArgs.strabsimusCorrection);
      GlUtils::renderSkybox(Resource::IMAGES_SKY_TRON_XNEG_PNG);
      mv.pop();
    }

    mv.push().transform(eyeArgs.modelviewOffset).preMultiply(eyeArgs.strabsimusCorrection);
    mv.preMultiply(glm::mat4_cast(predictionDelta));
    mv.scale(2.0f);

    gl::ProgramPtr videoRenderProgram = GlUtils::getProgram(
      Resource::SHADERS_SIMPLETEXTURED_VS,
      Resource::SHADERS_TEXTURE_FS);
    videoRenderProgram->use();
    pr.apply(videoRenderProgram);
    mv.apply(videoRenderProgram);
    textures[currentTexture]->bind();
    videoGeometry->bindVertexArray();

    videoGeometry->draw();

    gl::Texture2d::unbind();
    gl::VertexArray::unbind();
    gl::Program::clear();

    mv.pop();
  }
};

RUN_OVR_APP(Example05);

