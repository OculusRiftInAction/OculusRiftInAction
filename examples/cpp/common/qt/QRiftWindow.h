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
#include <QtOpenGL/QGLWidget>

class QRiftWindow : public QWindow, public RiftRenderingApp {
  Q_OBJECT
  bool shuttingDown{ false };
  LambdaThread renderThread;
  TaskQueueWrapper tasks;
  QOpenGLContext * m_context;

public:

  QRiftWindow();
  virtual ~QRiftWindow();

  QOpenGLContext * context() {
    return m_context;
  }

  void start();

  // Should only be called from the primary thread
  void stop();

  void queueRenderThreadTask(Lambda task);

  void * getNativeWindow() {
    return (void*)winId();
  }

  void toggleOvrFlag(ovrHmdCaps flag) {
    int caps = ovrHmd_GetEnabledCaps(hmd);
    if (caps & flag) {
      ovrHmd_SetEnabledCaps(hmd, caps & ~flag);
    } else {
      ovrHmd_SetEnabledCaps(hmd, caps | flag);
    }
  }

private:
  virtual void renderLoop();

protected:
  virtual void setup();
};
