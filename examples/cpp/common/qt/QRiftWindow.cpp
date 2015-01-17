#include "Common.h"


#ifdef HAVE_QT

QRiftWindow::QRiftWindow() {
  setSurfaceType(QSurface::OpenGLSurface);

  QSurfaceFormat format;
  // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
  format.setDepthBufferSize(16);
  format.setStencilBufferSize(8);
  format.setVersion(3, 3);
  format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
  setFormat(format);

  m_context = new QOpenGLContext;
  m_context->setFormat(format);
  m_context->create();

  renderThread.setLambda([&] { renderLoop(); });
  bool directHmdMode = false;

#ifdef USE_RIFT
  // The ovrHmdCap_ExtendDesktop only reliably reports on Windows currently
  ON_WINDOWS([&] {
    directHmdMode = (0 == (ovrHmdCap_ExtendDesktop & hmd->HmdCaps));
  });
#endif

  if (!directHmdMode) {
    setFlags(Qt::FramelessWindowHint);
  }

  // FIXME 
  setFlags(Qt::FramelessWindowHint);

  show();

#ifdef USE_RIFT
  if (directHmdMode) {
    setFramePosition(QPoint(0, -1080));
    // FIXME iterate through all screens and move the window to the best one
  } else {
    setFramePosition(QPoint(hmd->WindowsPos.x, hmd->WindowsPos.y));
  }
  resize(hmd->Resolution.w, hmd->Resolution.h);

  // If we're in direct mode, attach to the window, then hide it
  if (directHmdMode) {
    void * nativeWindowHandle = (void*)(size_t)winId();
    if (nullptr != nativeWindowHandle) {
      ovrHmd_AttachToWindow(hmd, nativeWindowHandle, nullptr, nullptr);
    }
  }
#else
  setFramePosition(QPoint(0, -1080));
  // FIXME iterate through all screens and move the window to the best one
  resize(1920, 1080);
#endif

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
#else
  MatrixStack & mv = Stacks::modelview();
  MatrixStack & pr = Stacks::projection();
  perFrameRender();
  Stacks::withPush(pr, mv, [&] {
    // Set up the per-eye projection matrix
    pr.top() = glm::perspective(PI / 2.0f, (float)size().width() / (float)size().height(), 0.01f, 10000.0f);
    renderScene();
  });
#endif
}

static RateCounter rateCounter;

void QRiftWindow::renderLoop() {
  m_context->makeCurrent(this);
  setup();
  QCoreApplication* app = QCoreApplication::instance();
  while (!shuttingDown) {
    m_context->makeCurrent(this);
    // Process the Qt message pump to run the standard window controls
    if (app->hasPendingEvents())
      app->processEvents();
    tasks.drainTaskQueue();
    drawFrame();
#ifndef USE_RIFT
    m_context->swapBuffers(this);
    rateCounter.increment();
    if (rateCounter.count() > 60) {
      float fps = rateCounter.getRate();
      updateFps(fps);
      rateCounter.reset();
    }
#endif
  }
  m_context->doneCurrent();
  m_context->moveToThread(QApplication::instance()->thread());
}

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
}

//#include "QRiftWindow.moc"

#endif

