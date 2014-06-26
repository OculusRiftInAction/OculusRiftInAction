#include "Common.h"

using namespace OVR;
using namespace gl;

#include <opencv2/opencv.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#define AR_DEFAULT_PIXEL_FORMAT AR_PIXEL_FORMAT_BGR
#include <AR/ar.h>

#define TEXTURES 6
// #define LATENCY_TEST

class PositionalApp : public RiftApp {
protected:
  cv::VideoCapture videoCapture;
  glm::mat4 headsetMatrix;
  boost::mutex frameMutex;
  boost::thread videoCaptureThread;
  glm::uvec2 imageSize;
  glm::mat4 riftOrientation;
  glm::mat4 player;
  ARParam cparam;
  int patt_id;

public:

  PositionalApp() : videoCapture(0) {
    if (!videoCapture.grab()) {
      FAIL("Failed grab");
    }
    videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
    videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
    int fps = (int)videoCapture.get(CV_CAP_PROP_FPS);
    videoCapture.set(CV_CAP_PROP_FPS, 60);
    videoCapture.set(CV_CAP_PROP_FOCUS, 0);
    imageSize = glm::ivec2(
      videoCapture.get(CV_CAP_PROP_FRAME_WIDTH),
      videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT));

    {
      std::vector<char> data(Resources::getResourceSize(Resource::MISC_PATT_HIRO));
      Resources::getResourceData(Resource::MISC_PATT_HIRO, &data[0]);
      std::string path = Resources::getResourcePath(Resource::MISC_PATT_HIRO);
      if ((patt_id = arLoadPatt(path.c_str())) < 0) {
        FAIL("Could not load pattern data");
      }
    }

    // set the initial camera parameters
    {
      std::vector<char> data(Resources::getResourceSize(Resource::MISC_CAMERA_PARA_DAT));
      Resources::getResourceData(Resource::MISC_CAMERA_PARA_DAT, &data[0]);
      std::string path = Resources::getResourcePath(Resource::MISC_CAMERA_PARA_DAT);
      ARParam wparam;
      if (arParamLoad(path.c_str(), 1, &wparam) < 0) {
        FAIL("Could not load camera data");
      }
      arParamChangeSize(&wparam, imageSize.x, imageSize.y, &cparam);
      arInitCparam(&cparam);
      arParamDisp(&cparam);
    }

    sensorFusion.SetGravityEnabled(true);
    sensorFusion.SetPredictionEnabled(true);
    player = glm::inverse(glm::lookAt(glm::vec3(0, 1, 3), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0)));
  }


  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS == action || GLFW_REPEAT == action)  switch (key) {
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

    if (CameraControl::instance().onKey(player, key, scancode, action, mods)) {
      return;
    }
    GlfwApp::onKey(key, scancode, action, mods);
  }


  void initGl() {
    RiftApp::initGl();
    glEnable(GL_BLEND);
    videoCaptureThread = boost::thread(boost::ref(*this));
  }

  void operator ()() {
    cv::Mat frame;
    int framecount = 0;
    long start = Platform::elapsedMillis();
    while (!glfwWindowShouldClose(window)) {
      cv::waitKey(1);
      if (!videoCapture.grab()) {
        FAIL("Failed grab");
      }
      videoCapture.retrieve(frame);
      ARMarkerInfo    *marker_info;
      int             marker_num;
      int             thresh = 250;

//      static uchar* camData = new uchar[frame.total() * 4];
//      cv::Mat continuousRGBA(frame.size(), CV_8UC4, camData);
//      cv::cvtColor(frame, continuousRGBA, CV_BGR2RGBA, 4);
      int detectResult = arDetectMarker(frame.data, thresh, &marker_info, &marker_num);
      if (detectResult < 0) {
        SAY("Error calling detect marker");
        continue;
      } else if (detectResult != 0) {
        SAY("Detected %d items", detectResult);
      }

      {
        boost::lock_guard<boost::mutex> lock(frameMutex);
        for (int i = 0; i < marker_num; ++i) {
          if (marker_info->id == patt_id) {
            double          patt_width = 80.0;
            double          patt_center[2] = { 0.0, 0.0 };
            double          patt_trans[3][4];
            arGetTransMat(&marker_info[i], patt_center, patt_width, patt_trans);
            //SAY("%f %f %f\n", );
            headsetMatrix[0] = glm::vec4(patt_trans[0][0], patt_trans[1][0], patt_trans[2][0], 0);
            headsetMatrix[1] = glm::vec4(patt_trans[0][1], patt_trans[1][1], patt_trans[2][1], 0);
            headsetMatrix[2] = glm::vec4(patt_trans[0][2], patt_trans[1][2], patt_trans[2][2], 0);
            glm::vec3 translation(patt_trans[0][3], patt_trans[1][3], patt_trans[2][3]);
            headsetMatrix[3] = glm::vec4(translation / 1000.0f, 1);
            headsetMatrix = glm::scale(headsetMatrix, glm::vec3(1.0f / 1000.0f));
          }
        }
      }
    }
  }

  virtual void renderScene() {
    glEnable(GL_DEPTH_TEST);
    clearColor(glm::vec3(0.05f, 0.05f, 0.05f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 hmd;
    {
      boost::lock_guard<boost::mutex> lock(frameMutex);
      hmd = headsetMatrix;
    }

    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.with_push([&]{
      mv.translate(glm::vec3(hmd[3].x, hmd[3].y, -hmd[3].z));
      GlUtils::renderSkybox(Resource::IMAGES_SKY_TRON_XNEG_PNG);
      GlUtils::drawColorCube();
      GlUtils::renderFloorGrid(player);
    });

    //mv.scale(2.0f);
    //mv.pop();

    //mv.push().scale(0.2f);
    //mv.top() *= arMat;
    //GlUtils::drawColorCube();
    //mv.pop();
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

    //GlUtils::renderString(message, cursor, 12.0f);
    //mv.pop();
    //glEnable(GL_DEPTH_TEST);
    //pr.pop();

  }
};

RUN_OVR_APP(PositionalApp);

