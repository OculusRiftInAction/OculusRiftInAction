#include "Common.h"
#include <oglplus/shapes/plane.hpp>
#include <pxcsensemanager.h>
#include <pxccapture.h>
#include <pxcsession.h>

void CaptureAlignedColorDepthSamples(void) {
   // Create a SenseManager instance
   PXCSenseManager *sm=PXCSenseManager::CreateInstance();

   // Select the color and depth streams
   sm->EnableStream(PXCCapture::STREAM_TYPE_COLOR,640,480,30);
   sm->EnableStream(PXCCapture::STREAM_TYPE_DEPTH,320,240,30);

   // Initialize and Stream Samples
   sm->Init();
   for (;;) {
       // This function blocks until both samples are ready
       if (sm->AcquireFrame(true)<PXC_STATUS_NO_ERROR) break;

       // retrieve the color and depth samples aligned
       PXCCapture::Sample *sample=sm->QuerySample();

       // work on the samples sample->color and sample->depth

       // go fetching the next samples
       sm->ReleaseFrame();
   }

   // Close down
   sm->Release();
}

static const glm::uvec2 WINDOW_SIZE(1280, 800);
static const glm::ivec2 WINDOW_POS(100, 100);
static const glm::uvec2 RES(WINDOW_SIZE.x / 8, WINDOW_SIZE.y / 8);
static const glm::uvec2 RASTER_RES(RES.x * 2, RES.y * 2);

using namespace oglplus;

template <class T>
class ref_ptr : public std::shared_ptr<T> {
public:
  ref_ptr() { }
  ref_ptr(T * t) : std::shared_ptr<T>(t, [](T*t){ t->Release(); }) {}
};

class RealSenseApp : public GlfwApp {
  ref_ptr<PXCSession> session{ PXCSession::CreateInstance() };
  ref_ptr<PXCSenseManager> sm{ session->CreateSenseManager() };
public:
  RealSenseApp()  {
    PXCSession::ImplDesc desc1 = {};
    desc1.group = PXCSession::IMPL_GROUP_SENSOR;
    desc1.subgroup = PXCSession::IMPL_SUBGROUP_VIDEO_CAPTURE;
    //for (int m = 0;; m++) {
    //  PXCSession::ImplDesc desc2;
    //  if (session->QueryImpl(&desc1, m, &desc2)<PXC_STATUS_NO_ERROR) break;
    //  SAY("Module[%d]: %s\n", m, desc2.friendlyName);

    //  PXCCapture *capture;
    //  session->CreateImpl<PXCCapture>(&desc2, &capture);
    //  // print out all device information
    //  for (int d = 0;; d++) {
    //    PXCCapture::DeviceInfo dinfo;
    //    if (capture->QueryDeviceInfo(d, &dinfo)<PXC_STATUS_NO_ERROR) break;
    //    SAY("    Device[%d]: %s\n", d, dinfo.name);
    //    auto dd = sm->QueryCaptureManager()->QueryDevice();
    //  }
    //  capture->Release();
    //}
    // Select the color and depth streams
    auto status = sm->EnableStream(PXCCapture::STREAM_TYPE_COLOR, 640, 480, 60);
    status = sm->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, 640, 480, 60);

    // Initialize and Stream Samples
    sm->Init();
  }

  virtual ~RealSenseApp() {
    sm->Release();
  }

  GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    outSize = WINDOW_SIZE;
    return glfw::createSecondaryScreenWindow(WINDOW_SIZE);
  }
  virtual void draw() {
    glClearColor(0, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    if (sm->AcquireFrame(true) == PXC_STATUS_NO_ERROR) {
      PXCCapture::Sample *sample = sm->QuerySample();
      sample->color()->
      sm->ReleaseFrame();
    }
  }

};

RUN_APP(RealSenseApp);

