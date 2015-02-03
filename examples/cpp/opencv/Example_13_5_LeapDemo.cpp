#include "Config.h"
#ifdef HAVE_LEAP
#include <Leap.h>
#endif

#include "Common.h"

#ifdef HAVE_LEAP
#include <thread>
#include <mutex>



/**
 Minimal demo of using the Leap Motion controller.
 Unlike other demos, this file relies on the Leap SDK which is not
 provided on our Github repo.  The Leap SDK evolves rapidly, so
 downloading it and updating the three Leap-specific fields in your
 CMake config is left as an exercise for the reader.
*/

struct CaptureData {
  glm::mat4 leapPose;
  Leap::Frame frame;
};

class LeapHandler : public Leap::Listener {

private:

  bool hasFrame{ false };
  Leap::Controller controller;
  std::thread captureThread;
  std::mutex mutex;
  CaptureData frame;
  ovrHmd hmd;

public:

  LeapHandler(ovrHmd & hmd) : hmd(hmd) {
  }

  void startCapture() {
    controller.addListener(*this);
  }

  void stopCapture() {
    controller.removeListener(*this);
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

  void onConnect(const Leap::Controller & controller) {
    controller.setPolicyFlags(Leap::Controller::PolicyFlag::POLICY_OPTIMIZE_HMD);
  }

  void onFrame(const Leap::Controller & controller) {
    CaptureData frame;

    frame.frame = controller.frame();
    frame.leapPose = ovr::toGlm(ovrHmd_GetTrackingState(hmd, 0.0).HeadPose.ThePose);
    set(frame);
  }
};

class LeapApp : public RiftApp {

  const float BALL_RADIUS = 0.05f;

protected:

  LeapHandler     captureHandler;
  ShapeWrapperPtr sphere;
  ProgramPtr      program;

  CaptureData latestFrame;
  glm::vec3 ballCenter;

public:

  LeapApp() : captureHandler(hmd) {
    captureHandler.startCapture();
    ballCenter = glm::vec3(0, 0, -0.25);
  }

  virtual ~LeapApp() {
    captureHandler.stopCapture();
  }

  void initGl() {
    RiftApp::initGl();
    program = oria::loadProgram(Resource::SHADERS_LIT_VS, Resource::SHADERS_LITCOLORED_FS);
    sphere = oria::loadSphere({"Position", "Normal"}, program);
    oria::bindLights(program);
  }

  virtual void update() {
    if (captureHandler.get(latestFrame)) {
      Leap::HandList hands = latestFrame.frame.hands();
      for (int iHand = 0; iHand < hands.count(); iHand++) {
        Leap::Hand hand = hands[iHand];
        Leap::Finger finger = hand.fingers()[1];  // Index only
        if (hand.isValid() && finger.isExtended()) {
          moveBall(finger);
        }
      }
    }
  }

  void moveBall(Leap::Finger finger) {
    glm::vec3 riftCoords = leapToRiftPosition(finger.tipPosition());
    riftCoords = glm::vec3(latestFrame.leapPose * glm::vec4(riftCoords, 1));
    if (glm::length(riftCoords - ballCenter) <= BALL_RADIUS) {
      ballCenter.x += (riftCoords.x - ballCenter.x) / 4;
      ballCenter.y += (riftCoords.y - ballCenter.y) / 4;
    }
  }

  virtual void renderScene() {
    glClear(GL_DEPTH_BUFFER_BIT);
    oria::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    MatrixStack & mv = Stacks::modelview();

    mv.withPush([&]{
      mv.transform(latestFrame.leapPose);

      Leap::HandList hands = latestFrame.frame.hands();
      for (int iHand = 0; iHand < hands.count(); iHand++) {
        Leap::Hand hand = hands[iHand];
        if (hand.isValid()) {
          drawHand(hand);
        }
      }
    });

    //GlUtils::draw3dLine(glm::vec3(ballCenter.x, -1000, ballCenter.z), glm::vec3(ballCenter.x, 1000, ballCenter.z));
    //GlUtils::draw3dLine(glm::vec3(-1000, ballCenter.y, ballCenter.z), glm::vec3(1000, ballCenter.y, ballCenter.z));
    drawSphere(ballCenter, BALL_RADIUS);
  }

  void drawHand(const Leap::Hand & hand) {
    drawSphere(leapToRiftPosition(hand.wristPosition()), 0.02f);
    for (int f = 0; f < hand.fingers().count(); f++) {
      Leap::Finger finger = hand.fingers()[f];
      if (finger.isValid()) {
        drawFinger(finger, hand.isLeft());
      }
    }
  }

  void drawFinger(const Leap::Finger & finger, bool isLeft) {
    MatrixStack & mv = Stacks::modelview();
    for (int b = 0; b < 4; b++) {
      mv.withPush([&] {
        Leap::Bone bone = finger.bone((Leap::Bone::Type) b);
        glm::vec3 riftCoords = leapToRiftPosition(bone.center());
        float length = bone.length() / 1000;

        mv.translate(riftCoords);
        mv.transform(leapBasisToRiftBasis(bone.basis(), isLeft));
        mv.scale(glm::vec3(0.01, 0.01, length));
        oria::renderColorCube();
      });
    }
  }

  void drawSphere(glm::vec3 & pos, float radius) {
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&]{
      mv.translate(pos);
      mv.scale(radius);
      oria::renderGeometry(sphere, program);
    });
  }

  glm::mat4 leapBasisToRiftBasis(Leap::Matrix & mat, bool isLeft) {
    glm::vec3 x = leapToRift(mat.transformDirection(Leap::Vector(isLeft ? -1 : 1, 0, 0)));
    glm::vec3 y = leapToRift(mat.transformDirection(Leap::Vector(0, 1, 0)));
    glm::vec3 z = leapToRift(mat.transformDirection(Leap::Vector(0, 0, 1)));
    return glm::mat4x4(glm::mat3x3(x, y, z));
  }

  glm::vec3 leapToRift(Leap::Vector & vec) {
    return glm::vec3(-vec.x, -vec.z, -vec.y);
  }

  glm::vec3 leapToRiftPosition(Leap::Vector & vec) {
    return leapToRift(vec) / 1000.0f + glm::vec3(0, 0, -0.070);
  }
};

RUN_OVR_APP(LeapApp);
#else
MAIN_DECL {
  SAY("Leap Motion SDK required for this example");
}
#endif