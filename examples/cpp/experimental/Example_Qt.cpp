#include "Common.h"
#include "Shadertoy.h"
#include "Qt/GlslEditor.h"

using namespace oglplus;



#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class FontLoader {
public:
   FontLoader() {
    QFontDatabase::addApplicationFontFromData(qt::toByteArray(Resource::FONTS_INCONSOLATA_REGULAR_TTF));
  }
};

MAIN_DECL {
  int i = 1;
  QApplication app(i, &lpCmdLine);
  FontLoader _fl;

  QGraphicsScene scene;
  GlslEditor editor;
  {
    QWidget * dialog = new QWidget();
    dialog->setLayout(new QVBoxLayout());
    dialog->resize(1280, 720);
    dialog->layout()->addWidget(&editor);
    scene.addWidget(dialog);
  }

  QGraphicsView view;
  view.setViewport(new QWidget());
  view.resize(1280, 720);
  view.setScene(&scene);
  view.show();
  editor.setPlainText(qt::toString(Resource::SHADERTOY_SHADERS_4DF3DS_INFINITE_CITY_FS));
  app.exec();
}
//  qw.setSource(QUrl::fromLocalFile("C:/Users/bdavis/Git/OculusRiftExamples/resources/shadertoy/ChannelSelect.qml"));
//  QObject * item = qw.rootObject();
//  auto res = item->findChildren<QObject>();
//  for (int i = 0; i < res.count(); ++i) {
//    const QObject & o = res.at(i);
//    if (QString(o.metaObject()->className()) == QString("QQuickImage")) {
//      qDebug() << o.objectName() << " ";
//    }
//  }
//  MyClass myClass;
//  QObject::connect(item, SIGNAL(qmlSignal(QString)),
//                   &myClass, SLOT(cppSlot(QString)));
//
//
////  QGraphicsScene gs;
////  gs.addWidget(&qw);
////  
////  QGraphicsView gv;
////  gv.resize(1280, 720);
//
//
////  QObject *rect = view.->findChild<QObject*>("tex00");
//  qw.show();
//  return app.exec();
//}


//  QDialog * dialog = new QDialog(0, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
//  dialog->setWindowTitle("Test");
//  dialog->setLayout(new QVBoxLayout());
//  for (int i = 0; i < 10; ++i) {
//    QPushButton * button = new QPushButton("Press me");
//    dialog->layout()->addWidget(button);
//  }
//  dialog->resize(640, 480);
//  scene.addWidget(dialog)->setPos(0, 0);
//
//  foreach (QGraphicsItem *item, scene.items()) {
//    item->setFlag(QGraphicsItem::ItemIsMovable);
//    item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
//  }


//  QGraphicsView view;
//  view.resize(1280, 720);
//  view.setScene(&scene);
//  view.show();


//class QRiftApplication2 : public QApplication, public RiftRenderingApp {
//  static int argc;
//  static char ** argv;
//
//  static QGLFormat & getFormat() {
//    static QGLFormat glFormat;
//    glFormat.setVersion(3, 3);
//    glFormat.setProfile(QGLFormat::CoreProfile);
//    glFormat.setSampleBuffers(true);
//    return glFormat;
//  }
//
//  QGLWidget widget;
//
//private:
//  virtual void * getRenderWindow() {
//    return (void*)widget.effectiveWinId();
//  }
//
//public:
//  QRiftApplication2() : QApplication(argc, argv), widget(getFormat()) {
//  }
//
//  virtual int run() {
//    QCoreApplication* app = QCoreApplication::instance();
//    widget.makeCurrent();
//    initGl();
//    while (QCoreApplication::instance())
//    {
//      // Process the Qt message pump to run the standard window controls
//      if (app->hasPendingEvents())
//        app->processEvents();
//
//      // This will render a frame on every loop. This is similar to the way 
//      // that OVR_Platform operates.
//      widget.makeCurrent();
//      draw();
//    }
//    return 0;
//  }
//
//  virtual ~QRiftApplication2() {
//  }
//};

//int QRiftApplication2::argc = 0;
//char ** QRiftApplication2::argv = nullptr;
//
//class ThisApp : public QRiftApplication2 {
//public:
//  ThisApp() {
//    Stacks::modelview().top() = glm::lookAt(vec3(0, OVR_DEFAULT_EYE_HEIGHT, 1), vec3(0, OVR_DEFAULT_EYE_HEIGHT, 0), Vectors::UP);
//  }
//
//  virtual void renderScene() {
//    if (getCurrentEye() == ovrEye_Left) {
//      Context::ClearColor(0, 0, 1, 1);
//
//    } else {
//      Context::ClearColor(1, 0, 0, 1);
//    }
//    Context::Clear().ColorBuffer().DepthBuffer();
//    oria::renderCubeScene(OVR_DEFAULT_IPD, OVR_DEFAULT_EYE_HEIGHT);
//  }
//
//};
//
//RUN_OVR_APP(ThisApp)
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
//
//#endif
//

#include "Example_Qt.moc"
