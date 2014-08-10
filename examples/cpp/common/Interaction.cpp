#include "Common.h"

#ifdef HAVE_SIXENSE
#include <sixense.h>
#include <sixense_math.hpp>
#include <sixense_utils/controller_manager/controller_manager.hpp>
#endif

#ifdef HAVE_SPNAV
#include <spnav.h>

glm::vec3 getTranslation(const spnav_event_motion & m, float scale = 1.0f / 5000.0f) {
  return glm::vec3(m.x, m.y, -m.z) * scale;
}

glm::quat getRotation(const spnav_event_motion & m, float scale = 1.0f / 3000.0f) {
  return glm::quat(glm::vec3(m.rx, m.ry, m.rz) * scale);
}
#endif

CameraControl::CameraControl()
    : joystickEnabled(true), hydraEnabled(false), spacemouseEnabled(false) {
}

CameraControl & CameraControl::instance() {
  static CameraControl instance_;
  return instance_;
}

void CameraControl::enableHydra(bool enable) {
  hydraEnabled = enable;
}

void CameraControl::enableSpacemouse(bool enable) {
  spacemouseEnabled = enable;
}

void CameraControl::enableJoystick(bool enable) {
  joystickEnabled = enable;
}


void translateCamera(glm::mat4 & camera, const glm::vec3 & delta) {
  // SAY("Translating %01.3f %01.3f %01.3f", delta.x, delta.y, delta.z);
  // Bring the vector into camera space coordinates
  glm::vec3 eyeDelta = glm::quat(camera) * delta;
  camera = glm::translate(glm::mat4(), eyeDelta) * camera;
}

void rotateCamera(glm::mat4 & camera, const glm::quat & rot) {
  camera = camera * glm::mat4_cast(rot);
}


void recompose(glm::mat4 & camera) {
  glm::vec4 t4 = camera[3];
  t4 /= t4.w;
  camera = glm::mat4_cast(glm::normalize(glm::quat(camera)));
  camera[3] = t4;
}

#ifdef HAVE_SIXENSE
static glm::quat getHydraOrientation(const sixenseControllerData & c) {
  // Bunny orientation
  glm::quat q = glm::quat(
      c.rot_quat[0],
      c.rot_quat[1],
      c.rot_quat[2],
      c.rot_quat[3]);
  glm::vec3 euler = glm::eulerAngles(q);
  std::swap(euler.x, euler.z);
  return glm::quat(euler * 2.0f * 3.14159f / 360.0f);
}
#endif

bool CameraControl::onKey(int key, int scancode, int action, int mods) {
  if (GLFW_PRESS != action && GLFW_RELEASE != action) {
    return false;
  }
  SAY("KEY %s: %c", (GLFW_PRESS == action) ? "pressed" : "released", key);
  int update = (GLFW_PRESS == action) ? 1 : 0;
  switch (key) {
  case GLFW_KEY_A:
    keyboardTranslate.x = -update; return true;
  case GLFW_KEY_D:
    keyboardTranslate.x = update; return true;
  case GLFW_KEY_W:
    keyboardTranslate.z = -update; return true;
  case GLFW_KEY_S:
    keyboardTranslate.z = update; return true;
  case GLFW_KEY_C:
    keyboardTranslate.y = -update; return true;
  case GLFW_KEY_F:
    keyboardTranslate.y = update; return true;
  case GLFW_KEY_UP:
    keyboardRotate.x = -update; return true;
  case GLFW_KEY_DOWN:
    keyboardRotate.x = update; return true;
  case GLFW_KEY_RIGHT:
    keyboardRotate.y = -update; return true;
  case GLFW_KEY_LEFT:
    keyboardRotate.y = update; return true;
  case GLFW_KEY_Q:
    keyboardRotate.z = -update; return true;
  case GLFW_KEY_E:
    keyboardRotate.z = update; return true;
  }
  return false;
}

#ifdef HAVE_SIXENSE
const char * CURRENT_HYRDA_INSTRUCTION = nullptr;
void controller_manager_setup_callback(sixenseUtils::ControllerManager::setup_step step) {
  SAY("Step %d", step);
  if (sixenseUtils::getTheControllerManager()->isMenuVisible()) {
    // Draw the instruction.
    CURRENT_HYRDA_INSTRUCTION = sixenseUtils::getTheControllerManager()->getStepString();
    SAY(CURRENT_HYRDA_INSTRUCTION);
  }
  else {
    CURRENT_HYRDA_INSTRUCTION = nullptr;
  }
}
#endif

void CameraControl::applyInteraction(glm::mat4 & camera) {
#ifdef HAVE_SIXENSE
  static bool hydraInitialized = false;
  if (!hydraInitialized) {
    int init = sixenseInit();
    sixenseSetActiveBase(0);
    sixenseUtils::getTheControllerManager()->setGameType(sixenseUtils::ControllerManager::ONE_PLAYER_TWO_CONTROLLER);
    sixenseUtils::getTheControllerManager()->registerSetupCallback(controller_manager_setup_callback);
    hydraInitialized = true;
  }
  if (hydraEnabled) {
    sixenseSetActiveBase(0);
    static sixenseAllControllerData acd;
    sixenseGetAllNewestData(&acd);
    sixenseUtils::getTheControllerManager()->update(&acd);
    int i = sixenseUtils::getTheControllerManager()->getIndex(
        sixenseUtils::IControllerManager::P1L);
    const sixenseControllerData & left = acd.controllers[i];
    i = sixenseUtils::getTheControllerManager()->getIndex(
        sixenseUtils::IControllerManager::P1R);
    const sixenseControllerData & right = acd.controllers[i];
    translateCamera(camera,
        glm::vec3(left.joystick_x, right.joystick_y, -left.joystick_y)
            / 100.0f);
    rotateCamera(camera,
        glm::angleAxis(-right.joystick_x /100.0f, glm::vec3(0, 1, 0)));
  }
#endif

#ifdef HAVE_SPNAV
  static int spnav = -2;
  static spnav_event event;
  if (-2 == spnav) {
    spnav = spnav_open();
  }
  if (spnav >= 0) {
    int eventType;
    while (0 != (eventType = spnav_poll_event(&event))) {
      SAY("event type %d", eventType);
      if (SPNAV_EVENT_MOTION == eventType) {
        spnav_event_motion & m = event.motion;
        glm::vec3 spaceTranslation = getTranslation(m);
        translateCamera(camera, spaceTranslation);
//        camera = glm::rotate(camera, (float)m.rx / 200.0f, GlUtils::X_AXIS);

        // We take the world Y axis and put it into the camera reference frame
        glm::vec3 yawAxis = glm::inverse(glm::quat(camera)) * GlUtils::Y_AXIS;
        camera = glm::rotate(camera, (float)m.ry / 200.0f, yawAxis);

//        if (abs(m.ry) >= 3 || abs(m.rx) >= 3) {
//          if (m.rx > m.ry) {
//          } else {
//            camera = glm::rotate(camera, (float)m.ry / 200.0f, GlUtils::Y_AXIS);
//          }
//
//        }
//        camera = glm::rotate(camera, (float)m.rx / 500.0f, GlUtils::X_AXIS);
//        glm::quat currentRotation(camera);
//        glm::quat inverse = glm::inverse(currentRotation);
//        camera = glm::mat4_cast(inverse) * camera;
//        rot = glm::quat(euler);
//        camera = glm::mat4_cast(rot) * camera;
//        camera = glm::mat4_cast(spaceRotation * currentRotation) * camera;
//        rotateCamera(camera, glm::quat(rotation));
      }
    }
  }
//  int spnav_sensitivity(double sens);

#endif
  static bool joysetup = false;
  static bool x52present = false;
  static std::shared_ptr<GlfwJoystick> joystick;

  if (!joysetup) {
    joysetup = true;
    for (int i = 0; i < 10; ++i) {
      if (glfwJoystickPresent(i)) {
        const char * joyName = glfwGetJoystickName(i);
        x52present =
          std::string(joyName).find("X52") !=
          std::string::npos;
        joystick = std::shared_ptr<GlfwJoystick>(
          x52present ?
          (GlfwJoystick*)new SaitekX52Pro::Controller(i) :
          (GlfwJoystick*)new Xbox::Controller(i)
        );
        break;
      }
    }
  }

  if (joystick) {
    joystick->read();
    glm::vec3 translation;
    glm::quat rotation;
    float scale = 500.0f;

    if (x52present) {
      using namespace SaitekX52Pro::Axis;
      // 0 - 9
      float scaleMod = joystick->getCalibratedAxisValue(SaitekX52Pro::Axis::THROTTLE_SLIDER);
      scaleMod *= 0.5f;
      scaleMod += 0.5f;
      scaleMod *= 9.0f;
      scaleMod += 1.0f;
      scale /= scaleMod;

      translation = joystick->getCalibratedVector(
          STICK_POV_X,
          STICK_POV_Y,
          THROTTLE_MAIN);

      glm::vec3 euler = joystick->getCalibratedVector(
          STICK_Y, STICK_Z, STICK_X);
      rotation = glm::quat(euler / 20.0f);
    } else {
      using namespace Xbox::Axis;
      translation.z = joystick->getCalibratedAxisValue(LEFT_Y) * 20.0f;
      translation.x = joystick->getCalibratedAxisValue(LEFT_X) * 20.0f;
      rotation.y = joystick->getCalibratedAxisValue(RIGHT_X) / 100.0f;
      rotation.x = joystick->getCalibratedAxisValue(RIGHT_Y) / 200.0f;
      rotation.z = joystick->getCalibratedAxisValue(TRIGGER) / 400.0f;
    }

    if (glm::length(translation) > 0.01f) {
      translateCamera(camera, translation / scale);
    }
    rotateCamera(camera, rotation);
    recompose(camera);
  }

  static uint32_t lastKeyboardUpdateTick = 0;
  uint32_t now = Platform::elapsedMillis();
  if (0 != lastKeyboardUpdateTick) {
    float dt = (now - lastKeyboardUpdateTick) / 1000.0f;
    if (keyboardRotate.x || keyboardRotate.y || keyboardRotate.z) {
      const glm::quat delta = glm::quat(glm::vec3(keyboardRotate) * dt);
      rotateCamera(camera, delta);
    }
    if (keyboardTranslate.x || keyboardTranslate.y || keyboardTranslate.z) {
      const glm::vec3 delta = glm::vec3(keyboardTranslate) * dt;
      translateCamera(camera, delta);
    }
  }
  lastKeyboardUpdateTick = now;
}

//
//  glm::quat leftQuat = getHydraOrientation(left);
//  glm::mat4 leftOrient = glm::mat4_cast(leftQuat);
//  glm::mat4 cameraOrient = glm::mat4_cast(glm::inverse(camera.getOrientation()));
//  leftOrient = cameraOrient * leftOrient;
//  curOrient = glm::quat(leftOrient);
//
//  leftQuat = camera.getOrientation() * curOrient;
//euler.y = -euler.y;
//  SAY("roll %03.2f, pitch %03.2f, yaw %03.2f", euler.z, euler.x, euler.y);
//  glm::vec3 euler = glm::eulerAngles();
//curOrient = camera.getOrientation() * curOrient;
//
//
//  curOrient = glm::angleAxis(PI, GlUtils::Z_AXIS) * curOrient;
//  curPos = glm::vec3(
//      left.pos[0],
//      left.pos[1],
//      left.pos[2]);
//
//void controller_manager_setup_callback(
//  sixenseUtils::ControllerManager::setup_step step) {
//if (sixenseUtils::getTheControllerManager()->isMenuVisible()) {
//
//  // Draw the instruction.
//  const char * instr =
//      sixenseUtils::getTheControllerManager()->getStepString();
//  SAY("Hydra controller manager: %s\n", instr);
//
//}
//}


//  int init = sixenseInit();
//
//  // Init the controller manager. This makes sure the controllers are present, assigned to left and right hands, and that
//  // the hemisphere calibration is complete.
//  sixenseUtils::getTheControllerManager()->setGameType(
//      sixenseUtils::ControllerManager::ONE_PLAYER_TWO_CONTROLLER);
//  sixenseUtils::getTheControllerManager()->registerSetupCallback(
//      controller_manager_setup_callback);
//  printf("Waiting for device setup...\n");
//  // Wait for the menu calibrations to go away... lazy man's menu, this
//  //  should be made graphical eventually
//  while (!sixenseUtils::getTheControllerManager()->isMenuVisible()) {
//    // update the controller manager with the latest controller data here
//    sixenseSetActiveBase(0);
//    sixenseGetAllNewestData(&acd);
//    sixenseUtils::getTheControllerManager()->update(&acd);
//  }
//
//  while (sixenseUtils::getTheControllerManager()->isMenuVisible()) {
//    // update the controller manager with the latest controller data here
//    sixenseSetActiveBase(0);
//    sixenseGetAllNewestData(&acd);
//    sixenseUtils::getTheControllerManager()->update(&acd);
//  }
