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

  // The ovrHmdCap_ExtendDesktop only reliably reports on Windows currently
  ON_WINDOWS([&] {
    directHmdMode = (0 == (ovrHmdCap_ExtendDesktop & hmd->HmdCaps));
  });

  if (!directHmdMode) {
    setFlags(Qt::FramelessWindowHint);
  }

  // FIXME 
  setFlags(Qt::FramelessWindowHint);

  show();

  if (directHmdMode) {
    setFramePosition(QPoint(0, -1080));
    // FIXME iterate through all screens and move the window to the best one
//    move(0, -1080);
  } else {
    setFramePosition(QPoint(hmd->WindowsPos.x, hmd->WindowsPos.y));
      // move(hmd->WindowsPos.x, hmd->WindowsPos.y);
  }
  resize(hmd->Resolution.w, hmd->Resolution.h);

  // If we're in direct mode, attach to the window, then hide it
  if (directHmdMode) {
    void * nativeWindowHandle = (void*)(size_t)winId();
    if (nullptr != nativeWindowHandle) {
      ovrHmd_AttachToWindow(hmd, nativeWindowHandle, nullptr, nullptr);
    }
  }
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


void QRiftWindow::renderLoop() {
  m_context->makeCurrent(this);
  setup();
  QCoreApplication* app = QCoreApplication::instance();
  while (!shuttingDown) {
    // Process the Qt message pump to run the standard window controls
    if (app->hasPendingEvents())
      app->processEvents();
    tasks.drainTaskQueue();
    drawRiftFrame();
  }
  m_context->doneCurrent();
  m_context->moveToThread(QApplication::instance()->thread());
}

void QRiftWindow::setup() {
  m_context->makeCurrent(this);
  glewExperimental = true;
  glewInit();

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

  initializeRiftRendering();
}


#endif

#include "QRiftWindow.moc"
