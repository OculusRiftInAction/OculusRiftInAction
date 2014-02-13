#include "Common.h"

using namespace OVR;
using namespace gl;
#include <glm/gtc/noise.hpp>
#include <opencv2/opencv.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#define AR_DEFAULT_PIXEL_FORMAT AR_PIXEL_FORMAT_BGR
#include <AR/ar.h>

#define TEXTURES 6
// #define LATENCY_TEST


class WebcamApp : public RiftApp {
protected:
  size_t currentTexture;
  gl::Texture2dPtr textures[TEXTURES];

#ifdef LATENCY_TEST
  OVR::Ptr<OVR::LatencyTestDevice> ovrLatencyTester;
  OVR::Util::LatencyTest latencyTest;
  GLFWwindow * latencyWindow;
#endif

  // OpenCV stuff
  // The video capture device.  Grabs on to the first web 
  // camera it finds.
  cv::VideoCapture videoCapture;
  glm::mat4 arMat;
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
  glm::uvec2 imageSize;
  gl::GeometryPtr videoGeometry;

  glm::quat predictionDelta;
  glm::quat riftOrientation;
  float videoFps;
  bool strabsimusCorrectionEnabled;
  float webcamLatency;
  float riftLatency;
  ARParam cparam;
  int patt_id;

 
public:

  WebcamApp() : videoCapture(3), currentTexture(0), frameChanged(false),
    strabsimusCorrectionEnabled(true) {
    webcamLatency = 0.161f;
    riftLatency = 0.028f;
    videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
    videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
    int fps = videoCapture.get(CV_CAP_PROP_FPS);
    videoCapture.set(CV_CAP_PROP_FPS, 60);
    // Force minimum focus, and disable autofocus
    videoCapture.set(CV_CAP_PROP_FOCUS, 0);
    imageSize = glm::ivec2(
      videoCapture.get(CV_CAP_PROP_FRAME_WIDTH), 
      videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT));

    if ((patt_id = arLoadPatt("c:/users/bdavis/git/OculusRiftExamples/resources/misc/patt.hiro")) < 0) {
      FAIL("Could not load pattern data");
    }

    // set the initial camera parameters 
    {
      ARParam wparam;
      if (arParamLoad("c:/users/bdavis/git/OculusRiftExamples/resources/misc/camera_para.dat", 1, &wparam) < 0) {
        FAIL("Could not load camera data");
      }
      arParamChangeSize(&wparam, imageSize.x, imageSize.y, &cparam);
      arInitCparam(&cparam);
      arParamDisp(&cparam);
    }


    sensorFusion.SetGravityEnabled(true);
    sensorFusion.SetPredictionEnabled(true);
#ifdef LATENCY_TEST
    ovrLatencyTester = *ovrManager->EnumerateDevices<OVR::LatencyTestDevice>().CreateDevice();
    if (ovrLatencyTester) {
      latencyTest.SetDevice(ovrLatencyTester);
    }
#endif
  }

#ifdef LATENCY_TEST
  void createRenderingTarget() {
    RiftApp::createRenderingTarget();

    latencyWindow = glfwCreateWindow(1920, 1080, "glfw", nullptr, window);
    if (!latencyWindow) {
      FAIL("Unable to create rendering window");
    }
    glfwSetWindowPos(latencyWindow, 0, -1080);
  }

  virtual void draw() {
    OVR::Color colorToDisplay;
    //    glfwMakeContextCurrent(latencyWindow);
    //    glClearColor(0, 1, 0, 1);
    latencyTest.ProcessInputs();
    if (latencyTest.DisplayScreenColor(colorToDisplay)) {
      glm::vec4 color(colorToDisplay.R, colorToDisplay.G, colorToDisplay.B, colorToDisplay.A);
      gl::clearColor(color / 255.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    else {
      RiftApp::draw();
    }
    //    glfwSwapBuffers(latencyWindow);
    //    glfwMakeContextCurrent(window);
  }
#endif

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action && GLFW_REPEAT != action) {
      GlfwApp::onKey(key, scancode, action, mods);
      return;
    }
    switch (key) {
    case GLFW_KEY_P:
      sensorFusion.SetPredictionEnabled(!sensorFusion.IsPredictionEnabled());
      break;
    case GLFW_KEY_S:
      strabsimusCorrectionEnabled = !strabsimusCorrectionEnabled;
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
    glEnable(GL_BLEND);
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

      ARMarkerInfo    *marker_info;
      int             marker_num;
      int             thresh = 100;

      static uchar* camData = new uchar[frame.total() * 4];
      cv::Mat continuousRGBA(frame.size(), CV_8UC4, camData);
      cv::cvtColor(frame, continuousRGBA, CV_BGR2RGBA, 4);

      /* detect the markers in the video frame */
      if (arDetectMarker(continuousRGBA.data, thresh, &marker_info, &marker_num) < 0) {
        FAIL("Error calling detect marker");
      }

      for (int i = 0; i < marker_num; ++i) {
        if (marker_info->id == patt_id) {
          double          patt_width = 80.0;
          double          patt_center[2] = { 0.0, 0.0 };
          double          patt_trans[3][4];
          arGetTransMat(&marker_info[i], patt_center, patt_width, patt_trans);
//          SAY("%f %f %f\n", );
          glm::mat4 & mat = arMat;
          mat[0] = glm::vec4(patt_trans[0][0], patt_trans[1][0], patt_trans[2][0], 0);
          mat[1] = glm::vec4(patt_trans[0][1], patt_trans[1][1], patt_trans[2][1], 0);
          mat[2] = glm::vec4(patt_trans[0][2], patt_trans[1][2], patt_trans[2][2], 0);
          glm::vec3 translation(patt_trans[0][3], patt_trans[1][3], patt_trans[2][3]);
          mat[3] = glm::vec4(translation / 1000.0f, 1) ;
          glm::scale(mat, glm::vec3(1.0f / 1000.0f));
        }
      }

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
        videoFps = (float)framecount / elapsed;
        // SAY("Video Capture FPS: %0.2f\n", fps);
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

    riftOrientation = Rift::fromOvr(sensorFusion.GetPredictedOrientation(riftLatency));

#ifdef LATENCY_TEST
    latencyTest.ProcessInputs();
#endif

    glm::quat webcamRiftOrientation = Rift::fromOvr(sensorFusion.GetPredictedOrientation(webcamLatency));
    predictionDelta = webcamRiftOrientation * glm::inverse(Rift::fromOvr(sensorFusion.GetOrientation()));

    gl::Stacks::modelview().top() = glm::lookAt(glm::vec3(0, 0, 2.0f), glm::vec3(), GlUtils::Y_AXIS);
  }

  virtual void renderScene(const PerEyeArgs & eyeArgs) {
    glEnable(GL_DEPTH_TEST);
    clearColor(glm::vec3(0.05f, 0.05f, 0.05f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();

    pr.top() = eyeArgs.getProjection();
    // Skybox
    {
      mv.push().preMultiply(glm::mat4_cast(riftOrientation));
      if (strabsimusCorrectionEnabled) 
        mv.preMultiply(eyeArgs.strabsimusCorrection);
      GlUtils::renderSkybox(Resource::IMAGES_SKY_TRON_XNEG_PNG);
      mv.pop();
    }


    mv.push().transform(eyeArgs.modelviewOffset);
    if (strabsimusCorrectionEnabled)
      mv.preMultiply(eyeArgs.strabsimusCorrection);
    mv.preMultiply(glm::mat4_cast(predictionDelta));
    mv.scale(2.0f);

    //gl::ProgramPtr videoRenderProgram = GlUtils::getProgram(
    //  Resource::SHADERS_SIMPLETEXTURED_VS,
    //  Resource::SHADERS_TEXTURE_FS);
    //videoRenderProgram->use();
    //pr.apply(videoRenderProgram);
    //mv.apply(videoRenderProgram);
    //textures[currentTexture]->bind();
    //videoGeometry->bindVertexArray();

    //videoGeometry->draw();

    //gl::Texture2d::unbind();
    //gl::VertexArray::unbind();
    //gl::Program::clear();

    mv.pop();

    mv.push().scale(0.2f);
    mv.top() *= arMat;
    GlUtils::drawColorCube();
    mv.pop();
    glDisable(GL_DEPTH_TEST);
    
    pr.push().top() = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f) * eyeArgs.projectionOffset;
    mv.push().identity();
    glm::vec2 cursor(-0.4, 0.4);
    std::string message = Platform::format(
      "OpenGL FPS: %0.2f\n"
      "Vidcap FPS: %0.2f\n"
      "Prediction: %0.2fms\n", 
      fps, videoFps, 
      webcamLatency * 1000.0f);

    GlUtils::renderString(message, cursor, 12.0f);
    mv.pop();
    glEnable(GL_DEPTH_TEST);
    pr.pop();

  }
};

RUN_OVR_APP(WebcamApp);

