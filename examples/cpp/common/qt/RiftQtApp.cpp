#include "Common.h"

#ifdef HAVE_QT

void QRiftWidget::paintGL() {
}

void * QRiftWidget::getRenderWindow() {
  return (void*)effectiveWinId();
}

QGLFormat & QRiftWidget::getFormat() {
  static QGLFormat glFormat;
  glFormat.setVersion(3, 3);
  glFormat.setProfile(QGLFormat::CoreProfile);
  glFormat.setSampleBuffers(false);
  return glFormat;
}

QRiftWidget::QRiftWidget(QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f)
  : QGLWidget(getFormat(), parent, shareWidget, f) {
  initWidget();
}

QRiftWidget::QRiftWidget(QGLContext *context, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f)
  : QGLWidget(context, parent, shareWidget, f) {
  initWidget();
}

QRiftWidget::QRiftWidget(const QGLFormat& format, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f)
  : QGLWidget(format, parent, shareWidget, f) {
  initWidget();
}

QRiftWidget::~QRiftWidget() {
  stop();
}

void QRiftWidget::start() {
  context()->doneCurrent();
  context()->moveToThread(&renderThread);
  renderThread.start();
  renderThread.setPriority(QThread::HighestPriority);
}

// Should only be called from the primary thread
void QRiftWidget::stop() {
  if (!shuttingDown) {
    shuttingDown = true;
    renderThread.exit();
    renderThread.wait();
    context()->makeCurrent();
  }
}

void QRiftWidget::queueRenderThreadTask(Lambda task) {
  tasks.queueTask(task);
}

void QRiftWidget::initWidget() {
  setAutoBufferSwap(false);
  renderThread.setLambda([&] { renderLoop(); });
  bool directHmdMode = false;

  // The ovrHmdCap_ExtendDesktop only reliably reports on Windows currently
  ON_WINDOWS([&] {
    directHmdMode = (0 == (ovrHmdCap_ExtendDesktop & hmd->HmdCaps));
  });

  if (!directHmdMode) {
    setWindowFlags(Qt::FramelessWindowHint);
  }

  // FIXME 
  setWindowFlags(Qt::FramelessWindowHint);

  show();

  if (directHmdMode) {
    // FIXME iterate through all screens and move the window to the best one
    move(0, -1080);
  } else {
    move(hmd->WindowsPos.x, hmd->WindowsPos.y);
  }
  resize(hmd->Resolution.w, hmd->Resolution.h);

  // If we're in direct mode, attach to the window, then hide it
  if (directHmdMode) {
    void * nativeWindowHandle = (void*)(size_t)effectiveWinId();
    if (nullptr != nativeWindowHandle) {
      ovrHmd_AttachToWindow(hmd, nativeWindowHandle, nullptr, nullptr);
    }
  } 
}

void QRiftWidget::renderLoop() {
  context()->makeCurrent();
  setup();
  QCoreApplication* app = QCoreApplication::instance();
  while (!shuttingDown) {
    // Process the Qt message pump to run the standard window controls
    if (app->hasPendingEvents())
      app->processEvents();
    tasks.drainTaskQueue();
    if (isRenderingConfigured()) {
      draw();
    } else {
      QThread::msleep(4);
    }
  }
  context()->doneCurrent();
  context()->moveToThread(QApplication::instance()->thread());
}

void QRiftWidget::setup() {
  context()->makeCurrent();
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

  initGl();
  update();
}


#endif

#include "RiftQtApp.moc"