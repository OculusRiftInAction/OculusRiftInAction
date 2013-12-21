#include "Common.h"

#ifdef HAVE_SIXENSE
#include <sixense.h>
#include <sixense_math.hpp>
#include <sixense_utils/controller_manager/controller_manager.hpp>

void controller_manager_setup_callback(
    sixenseUtils::ControllerManager::setup_step step);

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
  SAY("Translating %01.3f %01.3f %01.3f", delta.x, delta.y, delta.z);
  // Bring the vector into camera space coordinates
  glm::vec3 eyeDelta = glm::quat(camera) * delta;
  camera = glm::translate(glm::mat4(), eyeDelta) * camera;
}

void rotateCamera(glm::mat4 & camera, const glm::quat & rot) {
  camera = camera * glm::mat4_cast(rot);
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

void CameraControl::applyInteraction(glm::mat4 & camera) {
#ifdef HAVE_SIXENSE
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
        glm::angleAxis(-right.joystick_x, glm::vec3(0, 1, 0)));
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

//  for (int i = GLFW_JOYSTICK_1; i < GLFW_JOYSTICK_LAST; ++i) {
//    SAY("%0d joystick active", glfwJoystickPresent(i));
//  }
  if (joystickEnabled && glfwJoystickPresent(0)) {
    int axisCount = 0, buttonCount = 0;
    float axes[32]; {
      memset(axes, 0, sizeof(float) * 32);
      const float * joyAxes = glfwGetJoystickAxes(0, &axisCount);
      memcpy(axes, joyAxes, sizeof(float) * std::min(32, axisCount));
    }
    const unsigned char * buttons = glfwGetJoystickButtons (0, &buttonCount);
    glm::vec3 translation(axes[0], -axes[4], -axes[1]);
    if (translation != glm::vec3()) {
      translateCamera(camera, translation / 100.0f);
    }
    glm::quat rotation = glm::angleAxis(axes[3], glm::vec3(0, 1, 0));
    if (rotation != glm::quat()) {
      rotateCamera(camera, rotation);
    }
  }
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
//  curOrient = glm::angleAxis(180.0f, GlUtils::Z_AXIS) * curOrient;
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
/*
*
*

package com.encom13.gl.scene;

import java.io.IOException;

import net.java.games.input.Component;
import net.java.games.input.Component.Identifier;
import net.java.games.input.Controller;
import net.java.games.input.Controller.Type;
import net.java.games.input.ControllerEnvironment;

import org.lwjgl.input.Keyboard;
import org.lwjgl.input.Mouse;

import com.encom13.input.connextion.SpaceNav;
import com.encom13.input.connextion.SpaceNav.ButtonEvent;
import com.encom13.input.connextion.SpaceNav.MotionEvent;
import com.encom13.input.oculus.fusion.SensorFusion;

import copied.com.jme3.math.Quaternion;
import copied.com.jme3.math.Vector2f;
import copied.com.jme3.math.Vector3f;

public class UserSceneInteraction {
private final Camera       camera;
private Controller         joy                      = null;
private SpaceNavListener   spnav                    = null;
private final SensorFusion fusion                   = new SensorFusion();

public float               joystickRotationScale    = 1 / 100.0f;
public float               joystickTranslationScale = 1 / 10.0f;

public UserSceneInteraction(Camera camera) {
this.camera = camera;
}

private static Vector2f fromMouseEvent(int x, int y) {
Vector2f p = new Vector2f(x, y);
p.multLocal(1 / 500.0f);
return p;
}

public void enableRift(boolean enable) {
//        try {
//            fusion.SetGravityEnabled(false);
////            com.encom13.input.oculus.RiftTracker.startListening(fusion.createHandler());
//        } catch (IOException e) {
//            throw new IllegalStateException();
//        }
}

public void enableJoystick(boolean enable) {
if ((this.joy != null) == enable) {
return;
} else if (enable) {
Controller[] cs = ControllerEnvironment.getDefaultEnvironment().getControllers();
// less weak
for (Controller c : cs) {
if (c.getType() == Type.GAMEPAD) {
joy = c;
break;
}
}
} else {
this.joy = null;
}
}

public void enableSpaceNavigator(boolean enable) {
if ((this.spnav != null) == enable) {
return;
} else if (enable) {
SpaceNav.addEventListener(new SpaceNavListener());
} else {
SpaceNav.removeEventListener(spnav);
this.spnav = null;
}
}

public class SpaceNavListener implements SpaceNav.EventListener {
@Override
public void motionEvent(MotionEvent event) {
float scale = 0.0005f;
Vector3f tr = new Vector3f(event.x * scale, event.y * scale, -event.z * scale);
if (tr.length() > 0.015f) {
camera.translate(new Vector3f(event.x * scale, event.y * scale, -event.z * scale));
}
scale = 0.00005f;
Vector3f rot = new Vector3f(event.rx * scale, event.ry * scale, 0);
if (rot.length() > 0.001) {
camera.rotate(new Quaternion().fromAngles(rot.x, rot.y, rot.z));
}
// camera.rotate(new Quaternion().fromAngles(event.rx * scale,
// event.ry * scale, -event.rz * scale * 4));
}

@Override
public void buttonEvent(ButtonEvent event) {
camera.reset();
}
}

public void shutdown() {
enableSpaceNavigator(false);
}

private static final float walkingSpeed = 0.005f;

// boolean keyDown = Keyboard.isKeyDown(Keyboard.KEY_X);

public void applyInputUpdates() {
// camera.setRotation(fusion.getPredictedOrientation());
{
Vector3f tr = new Vector3f();
if (Keyboard.isKeyDown(Keyboard.KEY_W)) {
tr.z -= walkingSpeed;
} else if (Keyboard.isKeyDown(Keyboard.KEY_S)) {
tr.z += walkingSpeed;
} else if (Keyboard.isKeyDown(Keyboard.KEY_A)) {
tr.x -= walkingSpeed;
} else if (Keyboard.isKeyDown(Keyboard.KEY_D)) {
tr.x += walkingSpeed;
}
camera.translate(tr);
}
while (Keyboard.next()) {
int key = Keyboard.getEventKey();
switch (key) {
case Keyboard.KEY_W:
camera.translate(new Vector3f(0, 0, -walkingSpeed));
break;
case Keyboard.KEY_A:
camera.translate(new Vector3f(-walkingSpeed, 0, 0));
break;
case Keyboard.KEY_S:
camera.translate(new Vector3f(0, 0, walkingSpeed));
break;
case Keyboard.KEY_D:
camera.translate(new Vector3f(walkingSpeed, 0, 0));
break;
// case Keyboard.KEY_R:
// try {
// if (Keyboard.isKeyDown(Keyboard.KEY_LSHIFT)) {
// RiftTracker.getInstance().stopRecording();
// } else {
// RiftTracker.getInstance().record(new File("rift1.json"));
// }
// } catch (IOException e) {
// throw new IllegalStateException(e);
// }
// break;
}
}

while (Mouse.next()) {
if (Mouse.isButtonDown(0)) {
camera.rotate(fromMouseEvent(-Mouse.getEventDX(), Mouse.getEventDY()));
} else if (Mouse.isButtonDown(1)) {
camera.translate(fromMouseEvent(Mouse.getEventDX(), Mouse.getEventDY()));
} else if (Mouse.isButtonDown(2)) {
// camera.zoom(new Point(-Mouse.getEventDX(),
// -Mouse.getEventDY()));
}
}

if (null != joy) {
joy.poll();
Component c = joy.getComponent(Identifier.Button._4);
if (c != null) {
if (c.getPollData() != 0) {
camera.reset();
}
}

{
c = joy.getComponent(Identifier.Axis.X);
Component d = joy.getComponent(Identifier.Axis.Y);
Vector2f rot = new Vector2f(-c.getPollData(), -d.getPollData());
rot.multLocal(joystickRotationScale);
if (rot.length() > 0.0053f) {
camera.rotate(rot);
}
}

{
Vector3f tr = new Vector3f(joy.getComponent(Identifier.Axis.RX).getPollData(), 0, joy.getComponent(
Identifier.Axis.RY).getPollData());
tr.multLocal(joystickTranslationScale);
if (tr.length() > 0.05f) {
camera.translate(tr);
}
}
}
}
}
*/

// This is the callback that gets registered with the sixenseUtils::controller_manager.
// It will get called each time the user completes one of the setup steps so that the game
// can update the instructions to the user. If the engine supports texture mapping, the
// controller_manager can prove a pathname to a image file that contains the instructions
// in graphic form.
// The controller_manager serves the following functions:
//  1) Makes sure the appropriate number of controllers are connected to the system.
//     The number of required controllers is designaged by the
//     game type (ie two player two controller game requires 4 controllers,
//     one player one controller game requires one)
//  2) Makes the player designate which controllers are held in which hand.
//  3) Enables hemisphere tracking by calling the Sixense API call
//     sixenseAutoEnableHemisphereTracking. After this is completed full 360 degree
//     tracking is possible.
