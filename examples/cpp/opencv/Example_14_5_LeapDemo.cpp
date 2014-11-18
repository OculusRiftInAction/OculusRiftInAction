#include <Leap.h>
#include <thread>
#include <mutex>

#include "Common.h"

/**
 Minimal demo of using the Leap Motion controller.
 Unlike other demos, this file relies on the Leap SDK which is not
 provided on our Github repo.  The Leap SDK evolves rapidly, so
 downloading it is left as an exercise for the reader.
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
    controller.setPolicy(Leap::Controller::PolicyFlag::POLICY_OPTIMIZE_HMD);
  }

  void onFrame(const Leap::Controller & controller) {
    CaptureData frame;

    frame.frame = controller.frame();
    frame.leapPose = Rift::fromOvr(ovrHmd_GetTrackingState(hmd, 0.0).HeadPose.ThePose);
    set(frame);
  }
};

class LeapApp : public RiftApp {

  const float BALL_RADIUS = 0.05f;

protected:

  LeapHandler captureHandler;
  gl::GeometryPtr cube;
  gl::GeometryPtr sphere;
  gl::ProgramPtr program;

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
    cube = GlUtils::getColorCubeGeometry();
    sphere = GlUtils::getSphereGeometry();
    program = GlUtils::getProgram(Resource::SHADERS_LITCOLORED_VS, Resource::SHADERS_LITCOLORED_FS);

    ovrhmd_EnableHSWDisplaySDKRender(hmd, false);
  }

  virtual void update() {
    if (captureHandler.get(latestFrame)) {
      Leap::HandList hands = latestFrame.frame.hands();
      for (int iHand = 0; iHand < hands.count(); iHand++) {
        Leap::Hand hand = hands[iHand];
        if (hand.isValid()) {
          Leap::Finger finger = hand.fingers()[1];  // Index only
          if (finger.isExtended()) {
            glm::vec3 riftCoords = leapToRiftPosition(finger.tipPosition());
            riftCoords = riftCoords + glm::vec3(0, 0, -0.070);
            riftCoords = glm::vec3(latestFrame.leapPose * glm::vec4(riftCoords, 1));
            if (glm::length(riftCoords - ballCenter) <= BALL_RADIUS) {
              ballCenter.y += (riftCoords.y - ballCenter.y) / 4;
              ballCenter.x += (riftCoords.x - ballCenter.x) / 4;
            }
          }
        }
      }
    }
  }

  virtual void renderScene() {
    glClear(GL_DEPTH_BUFFER_BIT);
    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    gl::MatrixStack & mv = gl::Stacks::modelview();

    mv.with_push([&]{
      mv.transform(latestFrame.leapPose);
      mv.translate(glm::vec3(0, 0, -0.070));

      Leap::HandList hands = latestFrame.frame.hands();
      for (int iHand = 0; iHand < hands.count(); iHand++) {
        Leap::Hand hand = hands[iHand];
        if (hand.isValid()) {
          mv.with_push([&]{
            glm::vec3 riftCoords = leapToRiftPosition(hand.wristPosition());

            mv.translate(riftCoords);
            mv.transform(leapToRiftRotation(hand.basis(), hand.isLeft()));
            mv.scale(0.02);
            GlUtils::renderGeometry(sphere, program);
          });

          Leap::Matrix handTransform = hand.basis();
          handTransform.origin = hand.palmPosition();
          handTransform = handTransform.rigidInverse();

          for (int f = 0; f < hand.fingers().count(); f++) {
            Leap::Finger finger = hand.fingers()[f];
            if (finger.isValid()) {
              for (int b = 0; b < 4; b++) {
                mv.with_push([&]{
                  Leap::Bone bone = finger.bone((Leap::Bone::Type) b);
                  glm::vec3 riftCoords = leapToRiftPosition(bone.center());
                  float length = bone.length() / 1000;

                  mv.translate(riftCoords);
                  mv.transform(leapToRiftRotation(bone.basis(), hand.isLeft()));
                  mv.scale(glm::vec3(0.01, 0.01, length));
                  GlUtils::renderGeometry(cube, program);
                });
              }
            }
          }
        }
      }
    });

    GlUtils::draw3dLine(glm::vec3(ballCenter.x, -1000, ballCenter.z), glm::vec3(ballCenter.x, 1000, ballCenter.z));
    GlUtils::draw3dLine(glm::vec3(-1000, ballCenter.y, ballCenter.z), glm::vec3(1000, ballCenter.y, ballCenter.z));
    mv.with_push([&]{
      mv.translate(ballCenter);
      mv.scale(BALL_RADIUS);
      GlUtils::renderGeometry(sphere, program);
    });
  }

  glm::mat4 leapToRiftRotation(Leap::Matrix & mat, bool isLeft) {
    glm::vec3 x = glm::normalize(leapToRiftDirection(mat.transformDirection(Leap::Vector(isLeft ? -1 : 1, 0, 0))));
    glm::vec3 y = glm::normalize(leapToRiftDirection(mat.transformDirection(Leap::Vector(0, 1, 0))));
    glm::vec3 z = glm::normalize(leapToRiftDirection(mat.transformDirection(Leap::Vector(0, 0, 1))));
    return glm::mat4x4(
      x.x, x.y, x.z, 0,
      y.x, y.y, y.z, 0,
      z.x, z.y, z.z, 0,
      0, 0, 0, 1);
  }

  glm::vec3 leapToRiftDirection(Leap::Vector & vec) {
    static glm::mat3 pivotFromHorizontalToVertical(
      1.0,  0.0, 0.0,
      0.0,  0.0, 1.0,
      0.0, -1.0, 0.0);
    return pivotFromHorizontalToVertical * glm::vec3(-vec.x, -vec.y, vec.z);
  }

  glm::vec3 leapToRiftPosition(Leap::Vector & vec) {
    return glm::vec3(-vec.x / 1000.0, -vec.z / 1000.0, -vec.y / 1000.0);
  }
};

RUN_OVR_APP(LeapApp);
