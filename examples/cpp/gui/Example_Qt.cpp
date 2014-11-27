#include "Common.h"
#include "Shadertoy.h"
#include <QQuickView>
#include <QQuickItem>
#include <QQuickImageProvider>
#include <QtQuickWidgets/QQuickWidget>

using namespace oglplus;

class MyClass : public QObject {
    Q_OBJECT

public slots:
  void cppSlot(const QString &msg) {
    qDebug() << "Called the C++ slot with message:" << msg;
  }
};

Resource resourceFromQmlId(const QString & id) {
  static QRegExp re("(cube|tex)/(\\d+)");
  static const QString CUBE("cube");
  static const QString TEX("tex");
  if (!re.exactMatch(id)) {
    return NO_RESOURCE;
  }
  QString type = re.cap(1);
  int index = re.cap(2).toInt();
  qDebug() << "Type " << type << " Index " << index;
  if (CUBE == type) {
    return shadertoy::CUBEMAPS[index];
  } else if (TEX == type) {
    return shadertoy::TEXTURES[index];
  }
  return NO_RESOURCE;
}

QImage loadImageResource(Resource res) {
  std::vector<uint8_t> data = Platform::getResourceByteVector(res);
  QImage image;
  image.loadFromData(data.data(), data.size());
  return image;
}

class MyImageProvider : public QQuickImageProvider {
public:
  MyImageProvider() : QQuickImageProvider(QQmlImageProviderBase::Image) {}

  virtual QImage requestImage(const QString & id, QSize * size, const QSize & requestedSize) {
    qDebug() << "Id: " << id;
    Resource res = resourceFromQmlId(id);
    if (NO_RESOURCE == res) {
      qWarning() << "Unable to find resource for image ID " << id;
      return QQuickImageProvider::requestImage(id, size, requestedSize);
    }
    return loadImageResource(res);
  }
};


#include "Example_Qt.moc"


int main(int argc, char ** argv) {
  QApplication app(argc, argv);
  const char * FILE ="/Users/bradd/git/OculusRiftInAction/resources/shadertoy/ChannelSelect.qml";
  
  MyImageProvider imgProvider;
  
  QQuickWidget qw;
  qw.engine()->addImageProvider("res", &imgProvider);
  qw.setSource(QUrl::fromLocalFile(FILE));
  QObject * item = qw.rootObject();
  auto res = item->findChildren<QObject>();
  for (int i = 0; i < res.count(); ++i) {
    const QObject & o = res.at(i);
    if (QString(o.metaObject()->className()) == QString("QQuickImage")) {
      qDebug() << o.objectName() << " ";
    }
  }
  MyClass myClass;
  QObject::connect(item, SIGNAL(qmlSignal(QString)),
                   &myClass, SLOT(cppSlot(QString)));


//  QGraphicsScene gs;
//  gs.addWidget(&qw);
//  
//  QGraphicsView gv;
//  gv.resize(1280, 720);


//  QObject *rect = view.->findChild<QObject*>("tex00");
  qw.show();
  return app.exec();
}


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



//// Set up the channel selection window
//{
//  ImageManager & im = CEGUI::ImageManager::getSingleton();
//  for (int i = 0; i < 17; ++i) {
//    std::string str = Platform::format("Tex%02d", i);
//    if (pShaderChannels->isChild(str)) {
//      std::string imageName = "Image" + str;
//      Resource res = TEXTURES[i];
//      Window * pChannelButton = pShaderChannels->getChild(str);
//      std::string file = Resources::getResourcePath(res);
//      im.addFromImageFile(imageName, file, "resources");
//      pChannelButton->setProperty("NormalImage", imageName);
//      pChannelButton->setProperty("PushedImage", imageName);
//      pChannelButton->setProperty("HoverImage", imageName);
//      pChannelButton->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
//        size_t channel = (size_t)pShaderChannels->getUserData();
//        std::string controlName = Platform::format("ButtonC%d", channel);
//        Window * pButton = pShaderEditor->getChild(controlName);
//        pButton->setProperty("NormalImage", imageName);
//        pButton->setProperty("PushedImage", imageName);
//        pButton->setProperty("HoverImage", imageName);
//        setChannelInput(channel, TEXTURE, res);
//        setUiState(EDIT);
//        return true;
//      });
//    }
//  }
//  for (int i = 0; i < 6; ++i) {
//    std::string str = Platform::format("Cube%02d", i);
//    if (pShaderChannels->isChild(str)) {
//      std::string imageName = "Image" + str;
//      Resource res = CUBEMAPS[i];
//      Window * pChannelButton = pShaderChannels->getChild(str);
//      std::string file = Resources::getResourcePath(res);
//      im.addFromImageFile(imageName, file, "resources");
//      pChannelButton->setProperty("NormalImage", imageName);
//      pChannelButton->setProperty("PushedImage", imageName);
//      pChannelButton->setProperty("HoverImage", imageName);
//      pChannelButton->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
//        size_t channel = (size_t)pShaderChannels->getUserData();
//        std::string controlName = Platform::format("ButtonC%d", channel);
//        Window * pButton = pShaderEditor->getChild(controlName);
//        pButton->setProperty("NormalImage", imageName);
//        pButton->setProperty("PushedImage", imageName);
//        pButton->setProperty("HoverImage", imageName);
//        setChannelInput(channel, CUBEMAP, res);
//        setUiState(EDIT);
//        return true;
//      });
//    }
//  }
//}


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
//
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
