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


class QRiftWindow : 
  public QWindow
#ifdef USE_RIFT
  , protected RiftRenderingApp 
#endif
{
  Q_OBJECT
  bool shuttingDown{ false };
  LambdaThread renderThread;
  TaskQueueWrapper tasks;
  QOpenGLContext * m_context;

protected:
  float texRes{ 1.0f };
  float eyeOffsetScale{ 1.0f };

public:

  QRiftWindow();
  virtual ~QRiftWindow();

  QOpenGLContext * context() {
    return m_context;
  }

  void makeCurrent() {
    m_context->makeCurrent(this);
  }

  void start();

  // Should only be called from the primary thread
  virtual void stop();

  void queueRenderThreadTask(Lambda task);

  void * getNativeWindow() {
    return (void*)winId();
  }

private:
  virtual void renderLoop();
  virtual void drawFrame();


protected:
  virtual void resizeEvent(QResizeEvent * ev);

  virtual void setup();

#ifndef USE_RIFT
  virtual void updateFps(float fps) {
  }

  virtual void perFrameRender() {

  }

  virtual void perEyeRender() {
  }
#endif

  vec2 textureSize() {
  #ifdef USE_RIFT
      return vec2(eyesParams[0].size);
  #else
      return vec2(size().width(), size().height());
  #endif
  }

  uvec2 renderSize() {
      return uvec2(texRes * textureSize());
  }

};
