/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Bradley Austin Davis. All Rights reserved.

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

#include "QtCommon.h"

#ifdef HAVE_QT

inline QRect getSecondaryScreenGeometry(const uvec2 & size) {
  QDesktopWidget desktop;
  const int primary = desktop.primaryScreen();
  int monitorCount = desktop.screenCount();
  int best = -1;
  QRect bestSize;
  for (int i = 0; i < monitorCount; ++i) {
    if (primary == i) {
      continue;
    }
    QRect geometry = desktop.screenGeometry(i);
    QSize screenSize = geometry.size();
    if (best < 0 && (screenSize.width() >= (int)size.x  && screenSize.height() >= (int)size.y)) {
      best = i;
      bestSize = geometry;
    }
  }

  if (best < 0) {
    best = primary;
  }
  return desktop.screenGeometry(best);
}

QRiftWindow::QRiftWindow() {
  setSurfaceType(QSurface::OpenGLSurface);

  QSurfaceFormat format;
  // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
  format.setDepthBufferSize(16);
  format.setStencilBufferSize(8);
  format.setSwapBehavior(QSurfaceFormat::SwapBehavior::SingleBuffer);
  auto swapBehavior = format.swapBehavior();
  format.setVersion(4, 3);
  format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
  setFormat(format);

  m_context = new QOpenGLContext;
  m_context->setFormat(format);
  m_context->create();
  swapBehavior = m_context->format().swapBehavior();


  renderThread.setLambda([&] { renderLoop(); });
  bool directHmdMode = false;
  show();

#ifdef USE_RIFT
  uvec2 windowSize = uvec2(hmd->Resolution.w / 4, hmd->Resolution.h / 4);
#else 
  uvec2 windowSize = uvec2(1280, 800);
#endif

  QRect geometry = getSecondaryScreenGeometry(windowSize);
  setFramePosition(geometry.topLeft());
  resize(QSize(windowSize.x, windowSize.y));
}

QRiftWindow::~QRiftWindow() {
  stop();
}

void QRiftWindow::start() {
  m_context->doneCurrent();
  m_context->moveToThread(&renderThread);
  renderThread.start();
  renderThread.setPriority(QThread::HighestPriority);
}

// Should only be called from the primary thread
void QRiftWindow::stop() {
  if (!shuttingDown) {
    shuttingDown = true;
    renderThread.exit();
    renderThread.wait();
    m_context->makeCurrent(this);
  }
}

void QRiftWindow::queueRenderThreadTask(Lambda task) {
  tasks.queueTask(task);
}


void QRiftWindow::drawFrame() {
#ifdef USE_RIFT
  drawRiftFrame();
  m_context->swapBuffers(this);
#else
  MatrixStack & mv = Stacks::modelview();
  MatrixStack & pr = Stacks::projection();
  perFrameRender();
  Stacks::withPush(pr, mv, [&] {
    // Set up the per-eye projection matrix
    float aspect = (float)size().width() / (float)size().height();
    pr.top() = glm::perspective(PI / 3.0f, aspect, 0.01f, 10000.0f);
    perEyeRender();
  });
#endif
}

void QRiftWindow::renderLoop() {
  m_context->makeCurrent(this);
  setup();

  while (!shuttingDown) {
    if (QCoreApplication::hasPendingEvents())
      QCoreApplication::processEvents();
    tasks.drainTaskQueue();

    m_context->makeCurrent(this);
    drawFrame();
#ifndef USE_RIFT
    m_context->swapBuffers(this);
    static RateCounter rateCounter;
    rateCounter.increment();
    if (rateCounter.elapsed() > 1.0f) {
      float fps = rateCounter.getRate();
      updateFps(fps);
      rateCounter.reset();
    }
#endif
  }
  m_context->doneCurrent();
  m_context->moveToThread(QApplication::instance()->thread());
}

static bool _setup = false;
void QRiftWindow::setup() {
  m_context->makeCurrent(this);
  glewExperimental = true;
  glewInit();

  GLenum error = glGetError();
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  GLuint unusedIds = 0;
  if (glDebugMessageCallback) {
    glDebugMessageCallback(oria::debugCallback, this);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
      0, &unusedIds, true);
  } else if (glDebugMessageCallbackARB) {
    glDebugMessageCallbackARB(oria::debugCallback, this);
    glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
      0, &unusedIds, true);
  }
  error = glGetError();
#ifdef USE_RIFT
  initializeRiftRendering();
#endif
  _setup = true;
  if (mirrorEnabled) {
      makeCurrent();
      if (!mirrorFbo) {
          mirrorFbo = ovr::MirrorFboPtr(new ovr::MirrorFramebufferWrapper(hmd));
      }
      mirrorFbo->Init(oria::qt::toGlm(size()));
  }
}


void QRiftWindow::resizeEvent(QResizeEvent * ev) {
    glm::uvec2 newSize = oria::qt::toGlm(ev->size());
    if (mirrorEnabled && _setup) {
        queueRenderThreadTask([=] {
            makeCurrent();
            if (!mirrorFbo) {
                mirrorFbo = ovr::MirrorFboPtr(new ovr::MirrorFramebufferWrapper(hmd));
            }
            mirrorFbo->Init(newSize);
        });
    }
}

//#include "QRiftWindow.moc"

#endif

