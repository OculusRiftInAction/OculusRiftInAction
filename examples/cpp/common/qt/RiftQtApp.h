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

class QRiftWidget : public QGLWidget, public RiftRenderingApp {
  Q_OBJECT
  bool shuttingDown{ false };
  LambdaThread renderThread;
  TaskQueueWrapper tasks;

  void paintGL();
  virtual void * getRenderWindow();

public:
  static QGLFormat & getFormat();

  explicit QRiftWidget(QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0);

  explicit QRiftWidget(QGLContext *context, QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0);

  explicit QRiftWidget(const QGLFormat& format, QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0);

  virtual ~QRiftWidget();

  void start();

  // Should only be called from the primary thread
  void stop();

  void queueRenderThreadTask(Lambda task);

private:
  void initWidget();

  virtual void renderLoop();

protected:
  virtual void setup();
};
