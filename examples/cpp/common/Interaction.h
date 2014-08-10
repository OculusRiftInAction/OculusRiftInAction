/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ************************************************************************************/

#pragma once

class CameraControl {
  bool hydraEnabled;
  bool spacemouseEnabled;
  bool joystickEnabled;
  glm::ivec3 keyboardTranslate;
  glm::ivec3 keyboardRotate;

  CameraControl();
  public:
  static CameraControl & instance();
  void applyInteraction(glm::mat4 & camera);
  void enableHydra(bool enable = true);
  void enableSpacemouse(bool enable = true);
  void enableJoystick(bool enable = true);
  bool onKey(int key, int scancode, int action, int mods);

};

struct AxisCalibration {
  float max;
  float min;
  float center;
  float deadzoneSize;
  bool invert;

  AxisCalibration(bool invert = false,
      float center = 0.0f,
      float deadzoneSize = 0.01f)
      : max(1), min(-1),
          center(center), deadzoneSize(deadzoneSize), invert(invert) {
  }

  float getCalibratedValue(float rawValue) const {
    float result = rawValue - center;
    float deadzoneHalf = deadzoneSize / 2.0f;
    float absResult = std::abs(result);
    if (absResult <= deadzoneHalf) {
      return 0.0f;
    }

    if (result > 0.0f) {
      result /= max - center;
    } else {
      result /= center - min;
    }

    if (invert) {
      result *= -1.0f;
    }
    return result;
  }
};

class GlfwJoystick {
protected:
  typedef std::shared_ptr<AxisCalibration> Ptr;
  typedef std::map<size_t, Ptr> Map;
  typedef Map::iterator Itr;
  typedef Map::const_iterator CItr;
  typedef std::vector<float> AxisVector;
  typedef std::vector<uint8_t> ButtonVector;
  const unsigned int glfwIndex;
  AxisVector axes;
  ButtonVector buttons;
  Map calibration;
  std::string name;

public:
  GlfwJoystick(unsigned int glfwIndex)
      : glfwIndex(glfwIndex) {
    name = glfwGetJoystickName(glfwIndex);
  }

  bool getButtonState(unsigned int button) {
    if (button >= buttons.size()) {
      SAY_ERR("Button %d not available", button);
      return false;
    }
    return buttons[button] == 0 ? false : true;
  }

  float getRawAxisValue(unsigned int axis) const {
    if (axis >= axes.size()) {
      SAY_ERR("Axis %d not available", axis);
      return 0;
    }
    return axes[axis];
  }

  float getCalibratedAxisValue(unsigned int axis) const {
    if (axis >= axes.size()) {
      SAY_ERR("Axis %d not available, only %d axes", axis, axes.size());
      return 0;
    }
    float result = axes[axis];
    if (calibration.count(axis)) {
      CItr itr = calibration.find(axis);
      result = itr->second->getCalibratedValue(result);
    }
    return result;
  }

  glm::vec3 getCalibratedVector(const glm::ivec3 & v) const {
    return glm::vec3(
        getCalibratedAxisValue(v.x),
        getCalibratedAxisValue(v.y),
        getCalibratedAxisValue(v.z));
  }

  glm::vec3 getCalibratedVector(
      unsigned int x,
      unsigned int y,
      unsigned int z) const {
    return getCalibratedVector(glm::ivec3(x, y, z));
  }

  void read() {
    if (glfwJoystickPresent(glfwIndex)) {
      int axisCount = 0;
      const float * joyAxes = glfwGetJoystickAxes(glfwIndex, &axisCount);
      if (axisCount != axes.size()) {
        axes.resize(axisCount);
      }
//
//      {
//        std::string axisReport;
//        for (int i = 0; i < axisCount; ++i) {
//          static char buffer[16];
//          sprintf(buffer, "%02d %0.2f  ", i, joyAxes[i]);
//          axisReport += buffer;
//        }
//        SAY("%s", axisReport.c_str());
//      }
//
      memcpy(&axes[0], joyAxes, sizeof(float) * axisCount);
      int buttonCount = 0;
      const uint8_t * joyButtons =
        glfwGetJoystickButtons(glfwIndex, &buttonCount);

      //    {
      //      std::string buttonReport;
      //      for (int i = 0; i < buttonCount; ++i) {
      //        static char buffer[16];
      //        sprintf(buffer, "%02d %d  ", i, joyButtons[i]);
      //        buttonReport += buffer;
      //      }
      //      SAY("%s", buttonReport.c_str());
      //    }

      if (buttons.size() != buttonCount) {
        buttons.resize(buttonCount);
      }
      memcpy(&buttons[0], joyButtons, sizeof(uint8_t) * buttonCount);
    }
  }
};

namespace Xbox {
namespace Axis {
enum Enum {
  NONE = -1,
  LEFT_X = 0,
  LEFT_Y = 1,

  TRIGGER = 2,

  RIGHT_Y = 3,
  RIGHT_X = 4,
};
} // namespace Axis

namespace Button {
enum Enum {
  BUTTON_A = 0,
  BUTTON_B = 1,
  BUTTON_X = 2,
  BUTTON_Y = 3,
  LEFT_BUTTON = 4,
  RIGHT_BUTTON = 5,
  SELECT = 6,
  START = 7,
  L3 = 8,
  R3 = 9,
  DPAD_UP = 10,
  DPAD_RIGHT = 11,
  DPAD_DOWN = 12,
  DPAD_LEFT = 13,
};
} // namespace Button

class Controller : public GlfwJoystick {
public:
  Controller(unsigned int glfwIndex) : GlfwJoystick(glfwIndex) {
    calibration[Axis::RIGHT_X] = Ptr(new AxisCalibration(true));
    calibration[Axis::RIGHT_Y] = Ptr(new AxisCalibration(true));
    calibration[Axis::LEFT_X] = Ptr(new AxisCalibration(false, 0, 0.05f));
    calibration[Axis::LEFT_Y] = Ptr(new AxisCalibration(false, 0, 0.05f));
  }
};

} // Namespace XBox

namespace SaitekX52Pro {
namespace Axis {
enum Enum {
  NONE = -1,
  STICK_X = 0,
  STICK_Y = 1,
  STICK_Z = 5,
  STICK_POV_X = 7,
  STICK_POV_Y = 8,

  THROTTLE_MAIN = 2,
  THROTTLE_DIAL_Y = 3,
  THROTTLE_DIAL_X = 4,
  THROTTLE_SLIDER = 6,
  MOUSE_X = 9,
  MOUSE_Y = 10,
};
} // namespace Axis

namespace Button {
enum Enum {
  // Stick buttons
  TRIGGER = 0,
  MISSLE = 1,
  PINKY = 5,
  BUTTON_A = 2,
  BUTTON_B = 3,
  BUTTON_C = 4,
  TRIGGER_FULL = 14,

  // Stick POV 2 buttons
  // (POV 1 is axes 7 & 8)
  STICK_POV2_U = 19,
  STICK_POV2_R = 20,
  STICK_POV2_D = 21,
  STICK_POV2_L = 22,

  // Stick switches
  SWITCH_1_U = 8,
  SWITCH_1_D = 9,
  SWITCH_2_U = 10,
  SWITCH_2_D = 11,
  SWITCH_3_U = 12,
  SWITCH_3_D = 13,

  // Stick mode dial
  MODE_1 = 27,
  MODE_2 = 28,
  MODE_3 = 29,

  // Throttle buttons
  BUTTON_D = 6,
  BUTTON_E = 7,
  BUTTON_I = 30,
  MOUSE_C = 15,

  THROTTLE_POV_U = 23,
  THROTTLE_POV_R = 24,
  THROTTLE_POV_D = 25,
  THROTTLE_POV_L = 26,

  THROTTLE_WHEEL_D = 16,
  THROTTLE_WHEEL_U = 17,
  THROTTLE_WHEEL_C = 18,

  // MFD UI
  MFD_START = 32,
  MFD_RESET = 33,

  MFD_WHEEL1_C = 31,
  MFD_WHEEL1_U = 34,
  MFD_WHEEL1_D = 35,

  MFD_WHEEL2_U = 36,
  MFD_WHEEL2_D = 37,
  MFD_WHEEL2_C = 38,
};
} // Namespace Button

class Controller : public GlfwJoystick {
public:
  Controller(unsigned int glfwIndex) : GlfwJoystick(glfwIndex) {
    calibration[Axis::THROTTLE_MAIN] = Ptr(new AxisCalibration(true, 0.75f, 0.02f));
    calibration[Axis::STICK_Z] = Ptr(new AxisCalibration());
  }
};

} // Namespace SaitekX52Pro
