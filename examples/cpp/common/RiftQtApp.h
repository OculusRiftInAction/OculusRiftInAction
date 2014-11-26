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

#ifdef HAVE_QT

class QRiftApplication : public QApplication, public RiftRenderingApp {
  static int argc;
  static char ** argv;

  static QGLFormat & getFormat() {
    static QGLFormat glFormat;
    glFormat.setVersion(3, 3);
    glFormat.setProfile(QGLFormat::CoreProfile);
    glFormat.setSampleBuffers(true);
    return glFormat;
  }

  PaintlessGlWidget widget;

private:
  virtual void * getRenderWindow() {
    return (void*)widget.effectiveWinId();
  }

public:
  QRiftApplication() : QApplication(argc, argv), widget(getFormat()) {
    // Move to RiftUtils static method
    bool directHmdMode = false;
    widget.move(hmdDesktopPosition.x, hmdDesktopPosition.y);
    widget.resize(hmdNativeResolution.x, hmdNativeResolution.y);
    bool db = widget.doubleBuffer();
    Stacks::modelview().top() = glm::lookAt(vec3(0, OVR_DEFAULT_EYE_HEIGHT, 1), vec3(0, OVR_DEFAULT_EYE_HEIGHT, 0), Vectors::UP);

    // The ovrHmdCap_ExtendDesktop only reliably reports on Windows currently
    ON_WINDOWS([&]{
      directHmdMode = (0 == (ovrHmdCap_ExtendDesktop & hmd->HmdCaps));
    });

    if (directHmdMode) {
      widget.move(0, -1080);
    }
    else {
      widget.setWindowFlags(Qt::FramelessWindowHint);
    }
    widget.show();

    // If we're in direct mode, attach to the window
    if (directHmdMode) {
      void * nativeWindowHandle = (void*)(size_t)widget.effectiveWinId();
      if (nullptr != nativeWindowHandle) {
        ovrHmd_AttachToWindow(hmd, nativeWindowHandle, nullptr, nullptr);
      }
    }
  }


  virtual int run() {
    QCoreApplication* app = QCoreApplication::instance();
    widget.makeCurrent();
    initGl();
    while (QCoreApplication::instance())
    {
      // Process the Qt message pump to run the standard window controls
      if (app->hasPendingEvents())
        app->processEvents();

      // This will render a frame on every loop. This is similar to the way 
      // that OVR_Platform operates.
      widget.makeCurrent();
      draw();
    }
    return 0;
  }

  virtual ~QRiftApplication() {
  }
};


/**
A class that takes care of the basic duties of putting an OpenGL
window on the desktop in the correct position so that it's visible
through the Rift.
*/
#if 0
class RiftWidget : public QGLWidget, public RiftManagerApp {
protected:
  GLFWmonitor * hmdMonitor;
  const bool fullscreen;
  bool fakeRiftMonitor{ false };

public:
  RiftWidget(bool fullscreen = false) : fullscreen(fullscreen) {
  }

  virtual ~RiftWidget() {
  }

  virtual GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    return ovr::createRiftRenderingWindow(hmd, outSize, outPosition);
  }

  using GlfwApp::viewport;
  virtual void viewport(ovrEyeType eye) {
    const glm::uvec2 & windowSize = getSize();
    glm::ivec2 viewportPosition(eye == ovrEye_Left ? 0 : windowSize.x / 2, 0);
    GlfwApp::viewport(glm::uvec2(windowSize.x / 2, windowSize.y), viewportPosition);
  }
    
};

class RiftApp : public RiftGlfwApp {
public:

protected:
  glm::mat4 player;
  ovrTexture eyeTextures[2];
  ovrVector3f eyeOffsets[2];

private:
  ovrEyeRenderDesc eyeRenderDescs[2];
  ovrPosef eyePoses[2];
  ovrEyeType currentEye;

  glm::mat4 projections[2];
  FramebufferWrapper eyeFramebuffers[2];

protected:
  using RiftGlfwApp::renderStringAt;
  void renderStringAt(const std::string & str, float x, float y, float size = 18.0f);
  virtual void initGl();
  virtual void finishFrame();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void draw() final;
  virtual void postDraw() {};
  virtual void update();
  virtual void renderScene() = 0;

  virtual void applyEyePoseAndOffset(const glm::mat4 & eyePose, const glm::vec3 & eyeOffset);

  inline ovrEyeType getCurrentEye() const {
    return currentEye;
  }

  const ovrEyeRenderDesc & getEyeRenderDesc(ovrEyeType eye) const {
    return eyeRenderDescs[eye];
  }

  const ovrFovPort & getFov(ovrEyeType eye) const {
    return eyeRenderDescs[eye].Fov;
  }

  const glm::mat4 & getPerspectiveProjection(ovrEyeType eye) const {
    return projections[eye];
  }

  const ovrPosef & getEyePose(ovrEyeType eye) const {
    return eyePoses[eye];
  }

  const ovrPosef & getEyePose() const {
    return getEyePose(getCurrentEye());
  }

  const ovrFovPort & getFov() const {
    return getFov(getCurrentEye());
  }

  const ovrEyeRenderDesc & getEyeRenderDesc() const {
    return getEyeRenderDesc(getCurrentEye());
  }

  const glm::mat4 & getPerspectiveProjection() const {
    return getPerspectiveProjection(getCurrentEye());
  }

public:
  RiftApp(bool fullscreen = false);
  virtual ~RiftApp();
};
#endif
#endif
