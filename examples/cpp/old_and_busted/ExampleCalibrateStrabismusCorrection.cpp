#include "Common.h"

enum Step {
  INTRO,
  YAW,
  PITCH,
  ROLL,
  DONE,
};

const glm::vec3 CAMERA_START = glm::vec3(0, 1, 2);

class CalibrateStrabismusCorrection : public RiftApp {
protected:
  glm::vec3 correctEuler;
  Step step{INTRO};
  glm::mat4 player;
  glm::quat ref;
  glm::quat offset;
  gl::GeometryPtr geometry;
  glm::quat rift;
  Text::FontPtr font;

public:

  void initGl() {
    RiftApp::initGl();
    glEnable (GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_ERROR;

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    GL_CHECK_ERROR;
    font = GlUtils::getDefaultFont();

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

  glm::quat getOrientation() const {
    ovrPosef curPose = ovrHmd_GetSensorState(hmd, 0).Recorded.Pose;
    return Rift::fromOvr(curPose.Orientation);
  }

  void onKey(int key, int scancode, int action, int mods) {
    gl::MatrixStack & mv = gl::Stacks::modelview();

    glm::quat curOrientation = getOrientation();
    if (GLFW_PRESS == action) switch (key) {
    case GLFW_KEY_SPACE:
      switch (step) {
      case INTRO:
        step = PITCH;
        ref = curOrientation;
        break;

      case PITCH:
        step = YAW;
        ref = curOrientation;
        break;

      case YAW:
        step = DONE;
//        step = ROLL;
//        ref = curOrientation;
//        break;
//
//      case ROLL:
//        step = DONE;

        Rift::setStrabismusCorrection(offset);
        mv.identity();
        ovrHmd_ResetSensor(hmd);
        player = glm::inverse(glm::lookAt(CAMERA_START, glm::vec3(0, 0.5f, 0), GlUtils::Y_AXIS));
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
      ref = curOrientation;
      ovrHmd_ResetSensor(hmd);
      player = glm::inverse(glm::lookAt(CAMERA_START, glm::vec3(0, 0.5f, 0), GlUtils::Y_AXIS));
      return;
    }

    if (CameraControl::instance().onKey(player, key, scancode, action, mods)) {
      return;
    }
    RiftApp::onKey(key, scancode, action, mods);
  }

  virtual void update() {
    rift = getOrientation();

    // Find the difference between the current orientation and the reference
    glm::quat distance = rift * glm::inverse(ref);
    glm::vec3 rawEuler = glm::eulerAngles(glm::slerp(glm::quat(), distance, 0.5f));
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
      break;
    }
  }

  glm::mat4 getStrabismusCorrection(ovrEyeType eye) {
    glm::quat strabismusCorrection = offset;
    if (ovrEye_Left != eye) {
      strabismusCorrection = glm::inverse(offset);
    }
    return glm::mat4_cast(strabismusCorrection);
  }

  virtual void renderCalibration(ovrEyeType eye) {
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();
    glm::vec2 cursor(-0.4f, 0.3f);

    switch (step) {
    case INTRO:
      {
        static std::string text(
            "This utility will allow you to calibrate your per-eye offset for "
            "correcting strabismus, potentially reducing double vision and/or "
            "the need for prismatic glasses in the Rift\n\n"
            "If you normally wear prismatic glasses you should remove them for "
            "this test.\n\n"
            "Press spacebar to continue");
        mv.identity();
        pr.push().top() =  getOrthographicProjection(eye);
        font->renderString(text, cursor, 10.0f, 0.7f);
        pr.pop();
      }
      return;

    case PITCH:
      mv.identity();
      pr.push().top() = getOrthographicProjection(eye);
      font->renderString("Look up and down until the lines are even\n\nPress spacebar to continue", cursor, 10.0f, 0.7f);
      pr.pop();
      mv.top() = glm::lookAt(glm::vec3(0, 5, 0),
          GlUtils::ORIGIN, GlUtils::Z_AXIS);
      break;

    case YAW:
      mv.identity();
      pr.push().top() = getOrthographicProjection(eye);
      font->renderString("Move your head horizontally left and right until the lines are even", cursor, 10.0f, 0.7f);
      pr.pop();

      mv.top() = glm::lookAt(glm::vec3(5, 0, 0),
          GlUtils::ORIGIN, GlUtils::Y_AXIS);
      break;

    case ROLL:
      mv.identity();
      pr.push().top() = getOrthographicProjection(eye);
      font->renderString("Move your head horizontally left and right until the lines are even", cursor, 10.0f, 0.7f);
      pr.pop();

      mv.top() = glm::lookAt(glm::vec3(0, 0, 5),
          GlUtils::ORIGIN,
          (ovrEye_Left == eye) ? -GlUtils::Y_AXIS : GlUtils::Y_AXIS);
      break;
    }

    mv.push().preMultiply(getStrabismusCorrection(eye));
    GlUtils::renderGeometry(
        geometry,
        GlUtils::getProgram(
            Resource::SHADERS_SIMPLE_VS,
            Resource::SHADERS_COLORED_FS
        ));
    mv.pop();
  }

  virtual void renderScene() {
    glEnable(GL_DEPTH_TEST);
    gl::clearColor(glm::vec3(0.05f, 0.05f, 0.05f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (step != DONE) {
      renderCalibration(getCurrentEye());
      return;
    }


    gl::MatrixStack & mv = gl::Stacks::modelview();
    //glm::quat strabismusCorrection = offset;
    //if (eyeArgs.eye != LEFT) {
    //  strabismusCorrection = glm::inverse(offset);
    //}
    mv.with_push([&]{
      //mv.push().top() = getStrabismusCorrection(eyeArgs) *
      //  (glm::inverse(player) * glm::mat4_cast(rift));
      //mv.transform(eyeArgs.modelviewOffset);
      GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
      GlUtils::draw3dGrid();
      mv.scale(0.4f);
      mv.translate(glm::vec3(0, 0.5, 0));
      GlUtils::drawColorCube();
    });

//    glDisable(GL_DEPTH_TEST);
    std::string message = Platform::format(
      "Calibrated offset:\n %+0.3f vertical,\n %+0.3f lateral",
      correctEuler.x * RADIANS_TO_DEGREES,
      correctEuler.y * RADIANS_TO_DEGREES);
    renderStringAt(message, -0.4f, 0.5f);
  }
};

RUN_OVR_APP(CalibrateStrabismusCorrection);

