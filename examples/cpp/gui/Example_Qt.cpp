#include "Common.h"

using namespace oglplus;

//class DelegatingGlWidget : public QGLWidget {
//  typedef std::function<void()> Callback;
//  typedef std::function<void(int, int)> ResizeCallback;
//
//
//  Callback paintCallback;
//  Callback initCallback;
//  ResizeCallback resizeCallback;
//
//protected:
//  void initializeGL() {
//    initCallback();
//  }
//
//  void resizeGL(int w, int h) {
//    resizeCallback(w, h);
//  }
//
//  void paintGL() {
//    paintCallback();
//    update();
//  }
//
//  bool event(QEvent * e) {
//    if (QEvent::Paint == e->type()) {
//      return true;
//    }
//    QGLWidget::event(e);
//  }
//
//public:
//  explicit DelegatingGlWidget(const QGLFormat& format,
//    Callback paint, Callback init = []{}, ResizeCallback resize = [](int, int){}) :
//    QGLWidget(format), paintCallback(paint), initCallback(init), resizeCallback(resize) {
//    setAutoBufferSwap(false);
//  }
//};


class QRiftApplication2 : public QApplication, public RiftRenderingApp {
  static int argc;
  static char ** argv;

  static QGLFormat & getFormat() {
    static QGLFormat glFormat;
    glFormat.setVersion(3, 3);
    glFormat.setProfile(QGLFormat::CoreProfile);
    glFormat.setSampleBuffers(true);
    return glFormat;
  }

  QGLWidget widget;

private:
  virtual void * getRenderWindow() {
    return (void*)widget.effectiveWinId();
  }

public:
  QRiftApplication2() : QApplication(argc, argv), widget(getFormat()) {
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

  virtual ~QRiftApplication2() {
  }
};

int QRiftApplication2::argc = 0;
char ** QRiftApplication2::argv = nullptr;

class ThisApp : public QRiftApplication2 {
public:

  virtual void renderScene() {
    if (getCurrentEye() == ovrEye_Left) {
      Context::ClearColor(0, 0, 1, 1);

    } else {
      Context::ClearColor(1, 0, 0, 1);
    }
    Context::Clear().ColorBuffer().DepthBuffer();
    oria::renderCubeScene(OVR_DEFAULT_IPD, OVR_DEFAULT_EYE_HEIGHT);
  }

};

RUN_OVR_APP(ThisApp)

//class RiftWidget : public QGLWidget {
//
//public:
//
//  RiftWidget()
//    : QGLWidget(getFormat()) {
//    setAutoBufferSwap(false);
//    resize(1920, 1080);
//    move(0, -1080);
//    setWindowFlags(Qt::FramelessWindowHint);
//  }
//
//  virtual void viewport(ovrEyeType eye);
//
//
//  virtual ~RiftWidget() {
//  }
//
//private:
//  void initializeGL() {
//    SAY("INIT");
//
//    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
//    GLuint unusedIds = 0;
//    if (glDebugMessageCallback) {
//      glDebugMessageCallback(oria::debugCallback, this);
//      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
//        0, &unusedIds, true);
//    }
//    glGetError();
//  }
//
//
//
//    MyView view(&widget);
//    //    EventFilter ef(&view);
//    view.setViewport(new QWidget());
//    view.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
//    view.setScene(new MyScene());
//    //view.show();
//    view.resize(640, 480);
//    view.move(1920, 100);
//    return app.exec();
//  }
//};
//
//
//class MyScene : public QGraphicsScene {
//  Q_OBJECT
//public:
//  static MyScene * INSTANCE;
//  QWidget * uiWidget;
//  ShapeWrapperPtr shape;
//  ProgramPtr program;
//
//  
//  static QDialog * createDialog() {
//    QDialog * dialog = new QDialog(0, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
//    //dialog->setWindowOpacity(0.0);
//    dialog->setWindowTitle("Test");
//    dialog->setLayout(new QVBoxLayout());
//    return dialog;
//  }
//
//  MyScene() {
//    INSTANCE = this;
//    uiWidget = createDialog();
//    for (int i = 0; i < 30; ++i) {
//      QPushButton * button = new QPushButton("Press me");
//      uiWidget->layout()->addWidget(button);
//      connect(button, SIGNAL(pressed()), this, SLOT(customSlot()));
//    }
//    uiWidget->resize(640, 480);
//    addWidget(uiWidget)->setPos(0, 0);
//    foreach (QGraphicsItem *item, items()) {
//      item->setFlag(QGraphicsItem::ItemIsMovable);
//      item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
//    }
//  }
//  
//  private slots:
//    void customSlot() {
//      SAY("Click");
//    }
//};
//
//
//class EventFilter : public QWidget {
//  QObject * target;
//public:
//  EventFilter(QObject * target) :target(target) {
//
//  }
//
//  bool eventFilter(QObject *object, QEvent *event) {
//    if (QEvent::MouseButtonPress == event->type()) {
//      QApplication::sendEvent(target, event);
//      return true;
//    }
//    return false;
//  }
//};
//
//class MyApp {
//public:
//
//  int run() {
//    static int argc = 0;
//    static char ** argv = nullptr;
//    QApplication app(argc, argv);
//
//    QGLFormat glFormat;
//    
//    OculusWidget widget(glFormat);
//    widget.show();
//
//    
//
//    MyView view(&widget);
////    EventFilter ef(&view);
//    view.setViewport(new QWidget());
//    view.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
//    view.setScene(new MyScene());
//    //view.show();
//    view.resize(640, 480);
//    view.move(1920, 100);
//
//
//    return app.exec();
//  }
//};
//
//RUN_APP(MyApp);
//
//#include "Example_Qt.moc"
//
//#endif
//
