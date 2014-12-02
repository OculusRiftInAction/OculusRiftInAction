#include "Common.h"

using namespace glm;
using namespace gl;
enum Step {
  INTRO,
  YAW,
  PITCH,
  ROLL,
  DONE,
};

class CalibrateStrabismusCorrection : public RiftApp {
protected:
  Step step;
  float eyeHeight{ OVR_DEFAULT_EYE_HEIGHT };
  float ipd{ OVR_DEFAULT_IPD };
  glm::mat4 player;
  glm::quat ref;
  glm::quat offset;
  glm::quat rift;
  glm::vec2 cursor{ -0.4f, 0.3f };
  ovrHSWDisplayState hswState;

  gl::GeometryPtr geometry;
  bool flipCorrection{ false };
  bool enableCorrection{ true };

public:

  CalibrateStrabismusCorrection() : step(INTRO) {
    ipd = ovrHmd_GetFloat(hmd,
      OVR_KEY_IPD,
      OVR_DEFAULT_IPD);

    eyeHeight = ovrHmd_GetFloat(hmd,
      OVR_KEY_PLAYER_HEIGHT,
      OVR_DEFAULT_PLAYER_HEIGHT);

    resetPosition();
  }

  void initGl() {
    RiftApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_ERROR;

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    GL_CHECK_ERROR;

    // Create the test geometry
    {
      Mesh mesh;
      mesh.color = Colors::white;
      mesh.addVertex(GlUtils::X_AXIS * 0.2f);
      mesh.addVertex(GlUtils::X_AXIS * 2.0f);
      mesh.addVertex(-GlUtils::X_AXIS * 0.2f);
      mesh.addVertex(-GlUtils::X_AXIS * 2.0f);
      mesh.addVertex(GlUtils::Y_AXIS * 0.2f);
      mesh.addVertex(GlUtils::Y_AXIS * 2.0f);
      mesh.addVertex(-GlUtils::Y_AXIS * 0.2f);
      mesh.addVertex(-GlUtils::Y_AXIS * 2.0f);
      mesh.fillColors();
      geometry = mesh.getGeometry(GL_LINES);
    }
  }

  glm::quat getCurrentOrientation() {
    return ovr::toGlm(ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds()).HeadPose.ThePose.Orientation);
  }

  void setReferenceOrientation() {
    ref = getCurrentOrientation();
  }

  virtual void resetPosition() {
    static const glm::vec3 EYE = glm::vec3(0, eyeHeight, ipd * 5);
    static const glm::vec3 LOOKAT = glm::vec3(0, eyeHeight, 0);
    player = glm::inverse(glm::lookAt(EYE, LOOKAT, GlUtils::UP));
    ovrHmd_RecenterPose(hmd);
  }

  void onKey(int key, int scancode, int action, int mods) {

#ifndef _DEBUG
    if (hswState.Displayed) {
      ovrHmd_DismissHSWDisplay(hmd);
      return;
    }
#endif

    MatrixStack & mv = Stacks::modelview();

    if (GLFW_PRESS == action) switch (key) {
    case GLFW_KEY_SPACE:
      switch (step) {
      case INTRO:
        step = PITCH;
        setReferenceOrientation();
        break;

      case PITCH:
        step = YAW;
        setReferenceOrientation();
        break;

      case YAW:
        step = DONE;
//        step = ROLL;
//        ref = Rift::getQuaternion(sensorFusion);
//        break;
//
//      case ROLL:
//        step = DONE;

//        Rift::setStrabismusCorrection(offset);
        mv.identity();
        resetPosition();
        break;

      default:
        {
          glm::vec3 euler = glm::eulerAngles(offset);
          euler *= 2.0f;
          SAY("%0.2f, %0.2f", euler.x, euler.y);
        }
        break;
      }
      return;

    case GLFW_KEY_R:
      mv.identity();
      setReferenceOrientation();
      resetPosition();
      return;

    case GLFW_KEY_F:
      flipCorrection = !flipCorrection;
      return;

    case GLFW_KEY_E:
      enableCorrection = !enableCorrection;
      return;
    }

    if (CameraControl::instance().onKey(key, scancode, action, mods)) {
      return;
    }
    RiftApp::onKey(key, scancode, action, mods);
  }

  virtual void update() {
    ovrHmd_GetHSWDisplayState(hmd, &hswState);
    rift = getCurrentOrientation();

    // Find the difference between the current orientation and the reference
    glm::quat distance = rift * glm::inverse(ref);
    glm::vec3 rawEuler = glm::eulerAngles(slerp(glm::quat(), distance, 0.5f));
    static glm::vec3 correctEuler;
    switch (step) {
    case PITCH:
      correctEuler.x = rawEuler.x;
      offset = glm::quat(correctEuler);
      break;

    case YAW:
      correctEuler.y = rawEuler.y;
      offset = glm::quat(correctEuler);
      break;

    case ROLL:
      correctEuler.z = rawEuler.z;
//      offset = glm::quat(correctEuler);
      break;

    case DONE:
      CameraControl::instance().applyInteraction(player);
      MatrixStack & mv = Stacks::modelview();
      mv.top() = glm::inverse(player);
      break;
    }
  }

  glm::mat4 getStrabismusCorrection() {
    glm::quat strabismusCorrection = offset;
    ovrEyeType invertedEye = flipCorrection ? ovrEye_Left : ovrEye_Right;
    if (invertedEye == getCurrentEye()) {
      strabismusCorrection = glm::inverse(offset);
    }
    return glm::mat4_cast(strabismusCorrection);
  }

  virtual void renderCalibration() {
    MatrixStack & mv = Stacks::modelview();
    MatrixStack & pr = Stacks::projection();
    std::string text;
    switch (step) {
    case INTRO:
      {
        text = 
            "This utility will allow you to calibrate your\n"
            "per-eye offset for correcting strabismus, \n"
            "potentially reducing double vision and/or\n"
            "the need for prismatic glasses in the Rift\n\n"
            "If you normally wear prismatic glasses you\n"
            "should remove them for this test.\n\n"
            "Press spacebar to continue";
        renderStringAt(text, cursor.x, cursor.y, 10);
      }
      return;

    case PITCH:
      mv.identity();
      text = "Look up and down until the lines are even\n\nPress spacebar to continue";
      renderStringAt(text, cursor.x, cursor.y, 10);
      mv.top() = glm::lookAt(glm::vec3(0, 5, 0),
          GlUtils::ORIGIN, GlUtils::Z_AXIS);
      break;

    case YAW:
      mv.identity();
      text = "Turn your head\nleft and right until the lines are even\n\nPress spcaebar to continue";
      renderStringAt(text, cursor.x, cursor.y, 10);
      mv.top() = glm::lookAt(glm::vec3(5, 0, 0),
          GlUtils::ORIGIN, GlUtils::Y_AXIS);
      break;

    case ROLL:
      mv.identity();
      text = "Move your head horizontally left and right until the lines are even";
      renderStringAt(text, cursor.x, cursor.y, 10);
      //mv.top() = glm::lookAt(glm::vec3(0, 0, 5),
      //    GlUtils::ORIGIN,
      //    eyeArgs.left ? -GlUtils::Y_AXIS : GlUtils::Y_AXIS);
      break;
    }

    mv.push().preMultiply(getStrabismusCorrection());
    GlUtils::renderGeometry(
        geometry,
        GlUtils::getProgram(
            Resource::SHADERS_SIMPLE_VS,
            Resource::SHADERS_COLORED_FS
        ));
    mv.pop();
  }

  void applyEyePoseAndOffset(const glm::mat4 & eyePose, const glm::vec3 & eyeOffset) {
    MatrixStack & mv = Stacks::modelview();
    if (enableCorrection) {
      mv.preMultiply(getStrabismusCorrection());
    }
    mv.preMultiply(glm::inverse(eyePose));
    // Apply the per-eye offset
    mv.preMultiply(glm::translate(glm::mat4(), eyeOffset));
  }


  virtual void renderScene() {
#ifndef _DEBUG
    if (hswState.Displayed) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      return;
    }
#endif

    if (step != DONE) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      renderCalibration();
      return;
    }

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    GlUtils::renderCubeScene(ipd, eyeHeight);
    MatrixStack & mv = Stacks::modelview();
    mv.with_push([&]{
      mv.translate(glm::vec3(0, 0, ipd * -5));
      GlUtils::renderManikin();
    });
    if (enableCorrection && flipCorrection) {
      renderStringAt("Flipped", cursor.x, cursor.y, 10);
    }
  }
};

RUN_OVR_APP(CalibrateStrabismusCorrection);

