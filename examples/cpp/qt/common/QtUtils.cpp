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

#include <QDomDocument>
#include <QImage>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#else
#include <oglplus/images/png.hpp>
#endif


namespace oria {
  namespace qt {

    template<typename T>
    T toQtType(Resource res) {
      T result;
      size_t size = Resources::getResourceSize(res);
      result.resize((int) size);
      Resources::getResourceData(res, result.data());
      return result;
    }

    vec2 toGlm(const QSize & size) {
      return vec2(size.width(), size.height());
    }

    vec2 toGlm(const QPointF & pt) {
      return vec2(pt.x(), pt.y());
    }

    QSize sizeFromGlm(const vec2 & size) {
      return QSize(size.x, size.y);
    }

    QPointF pointFromGlm(const vec2 & pt) {
      return QPointF(pt.x, pt.y);
    }

    QByteArray toByteArray(Resource res) {
      return toQtType<QByteArray>(res);
    }

    QString toString(Resource res) {
      QByteArray data = toByteArray(res);
      return QString::fromUtf8(data.data(), data.size());
    }

    //QImage loadImageResource(Resource res) {
    //  QImage image;
    //  image.loadFromData(toByteArray(res));
    //  return image;
    //}
  }
}

QJsonValue path(const QJsonValue & parent, std::initializer_list<QVariant> elements) {
  QJsonValue current = parent;
  std::for_each(elements.begin(), elements.end(), [&](const QVariant & element) {
    if (current.isObject()) {
      QString path = element.toString();
      current = current.toObject().value(path);
    } else if (current.isArray()) {
      int offset = element.toInt();
      current = current.toArray().at(offset);
    } else {
      qWarning() << "Unable to continue";
      current = QJsonValue();
    }
  });
  return current;
}

typedef std::list<QString> List;
typedef std::map<QString, List> Map;
typedef std::pair<QString, List> Pair;

template <typename F>
void for_each_node(const QDomNodeList & list, F f) {
  for (int i = 0; i < list.size(); ++i) {
    f(list.at(i));
  }
}

static Map createGlslMap() {
  using namespace std;
  Map listMap;
  map<QString, Map> contextMapMap;
  QDomDocument document;
  {
    std::vector<uint8_t> data = Platform::getResourceByteVector(Resource::MISC_GLSL_XML);
    document.setContent(QByteArray((const char*)&data[0], (int)data.size()));
  }
  QDomElement s = document.documentElement().firstChildElement();
  for_each_node(s.childNodes(), [&](QDomNode child) {
    if (QString("list") == child.nodeName()) {
      QString listName = child.attributes().namedItem("name").nodeValue();
      list<QString> & l = listMap[listName];
      for_each_node(child.childNodes(), [&](QDomNode item) {
        if (QString("item") == item.nodeName()) {
          QString nodeValue = item.firstChild().nodeValue();
          l.push_back(item.firstChild().nodeValue());
        }
      });
    }


    if (QString("contexts") == child.nodeName()) {
      for_each_node(child.childNodes(), [&](QDomNode child) {
        if (QString("context") == child.nodeName()) {
          QString contextName = child.attributes().namedItem("name").nodeValue();
          qDebug() << "Context name: " << contextName;
          map<QString, list<QString>> & contextMap = contextMapMap[contextName];
          for_each_node(child.childNodes(), [&](QDomNode child) {
            if (QString("keyword") == child.nodeName()) {
              QString attribute = child.attributes().namedItem("attribute").nodeValue();
              QString value = child.attributes().namedItem("String").nodeValue();
              contextMap[attribute].push_back(value);
            }
          });
        }
      });
    }
  });

  Map finalMap;

  Map contextMap = contextMapMap["v330"];
  std::for_each(contextMap.begin(), contextMap.end(), [&](const Pair & maptype) {
    QString type = maptype.first;
    List & typeList = finalMap[type];
    foreach(const QString & listName, maptype.second) {
      const List & l = listMap[listName];
      typeList.insert(typeList.end(), l.begin(), l.end());
    }
  });

  foreach(const Pair & p, finalMap) {
    qDebug() << p.first;
    foreach(const QString & s, p.second) {
      qDebug() << "\t" << s;
    }
  }

  return finalMap;
}



QOffscreenUi::QOffscreenUi() {
}

QOffscreenUi::~QOffscreenUi() {
  // Make sure the context is current while doing cleanup. Note that we use the
  // offscreen surface here because passing 'this' at this point is not safe: the
  // underlying platform window may already be destroyed. To avoid all the trouble, use
  // another surface that is valid for sure.
  m_context->makeCurrent(m_offscreenSurface);

  // Delete the render control first since it will free the scenegraph resources.
  // Destroy the QQuickWindow only afterwards.
  delete m_renderControl;

  delete m_qmlComponent;
  delete m_quickWindow;
  delete m_qmlEngine;

  m_context->doneCurrent();

  delete m_offscreenSurface;
  delete m_context;
}



void QOffscreenUi::setup(const QSize & size, QOpenGLContext * shareContext) {
  m_uiSize = oria::qt::toGlm(size);

  QSurfaceFormat format;
  // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
  format.setDepthBufferSize(16);
  format.setStencilBufferSize(8);
  format.setMajorVersion(3);
  format.setMinorVersion(3);
  format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
  m_context->setFormat(format);
  if (nullptr != shareContext) {
    m_context->setShareContext(shareContext);
  }
  m_context->create();

  // Pass m_context->format(), not format. Format does not specify and color buffer
  // sizes, while the context, that has just been created, reports a format that has
  // these values filled in. Pass this to the offscreen surface to make sure it will be
  // compatible with the context's configuration.
  m_offscreenSurface->setFormat(m_context->format());
  m_offscreenSurface->create();

  m_context->makeCurrent(m_offscreenSurface);

  // Create a QQuickWindow that is associated with out render control. Note that this
  // window never gets created or shown, meaning that it will never get an underlying
  // native (platform) window.
  QQuickWindow::setDefaultAlphaBuffer(true);
  m_quickWindow = new QQuickWindow(m_renderControl);
  m_quickWindow->setColor(QColor(255, 255, 255, 0));
  m_quickWindow->setFlags(m_quickWindow->flags() | static_cast<Qt::WindowFlags>(Qt::WA_TranslucentBackground));
  // Create a QML engine.
  m_qmlEngine = new QQmlEngine;
  if (!m_qmlEngine->incubationController())
    m_qmlEngine->setIncubationController(m_quickWindow->incubationController());
  m_qmlEngine->addImageProvider(QLatin1String("resources"), new QResourceImageProvider());

  // When Quick says there is a need to render, we will not render immediately. Instead,
  // a timer with a small interval is used to get better performance.
  m_updateTimer.setSingleShot(true);
  m_updateTimer.setInterval(5);
  connect(&m_updateTimer, &QTimer::timeout, this, &QOffscreenUi::updateQuick);

  // Now hook up the signals. For simplicy we don't differentiate between
  // renderRequested (only render is needed, no sync) and sceneChanged (polish and sync
  // is needed too).
  connect(m_renderControl, &QQuickRenderControl::renderRequested, this, &QOffscreenUi::requestRender);
  connect(m_renderControl, &QQuickRenderControl::sceneChanged, this, &QOffscreenUi::requestUpdate);

  m_qmlComponent = new QQmlComponent(m_qmlEngine);

  // Update item and rendering related geometries.
  m_quickWindow->setGeometry(0, 0, m_uiSize.x, m_uiSize.y);

  // Initialize the render control and our OpenGL resources.
  m_context->makeCurrent(m_offscreenSurface);
  m_renderControl->initialize(m_context);

}

QQmlContext * QOffscreenUi::qmlContext() {
  if (nullptr == m_rootItem) {
    return m_qmlComponent->creationContext();
  }
  return QQmlEngine::contextForObject(m_rootItem);
}

void QOffscreenUi::loadQml(const QUrl & qmlSource, std::function<void(QQmlContext*)> f) {
  m_qmlComponent->loadUrl(qmlSource);
  if (m_qmlComponent->isLoading())
    connect(m_qmlComponent, &QQmlComponent::statusChanged, this, &QOffscreenUi::run);
  else
    run();
}

void QOffscreenUi::requestUpdate() {
  m_polish = true;
  if (!m_updateTimer.isActive())
    m_updateTimer.start();
}

void QOffscreenUi::requestRender() {
  if (!m_updateTimer.isActive())
    m_updateTimer.start();
}

void QOffscreenUi::run() {
  disconnect(m_qmlComponent, SIGNAL(statusChanged(QQmlComponent::Status)), this, SLOT(run()));
  if (m_qmlComponent->isError()) {
    QList<QQmlError> errorList = m_qmlComponent->errors();
    foreach(const QQmlError &error, errorList)
      qWarning() << error.url() << error.line() << error;
    return;
  }

  QObject *rootObject = m_qmlComponent->create();
  if (m_qmlComponent->isError()) {
    QList<QQmlError> errorList = m_qmlComponent->errors();
    foreach(const QQmlError &error, errorList)
      qWarning() << error.url() << error.line() << error;
    return;
  }

  m_rootItem = qobject_cast<QQuickItem *>(rootObject);
  if (!m_rootItem) {
    qWarning("run: Not a QQuickItem");
    delete rootObject;
    return;
  }

  // The root item is ready. Associate it with the window.
  m_rootItem->setParentItem(m_quickWindow->contentItem());
  m_rootItem->setWidth(m_uiSize.x);
  m_rootItem->setHeight(m_uiSize.y);

  SAY("Finished setting up QML");
}

void QOffscreenUi::lockTexture(int texture) {
  Q_ASSERT(m_fboMap.count(texture));
  if (!m_fboLocks.count(texture)) {
    Q_ASSERT(m_readyFboQueue.front()->texture() == texture);
    m_readyFboQueue.pop_front();
    m_fboLocks[texture] = 1;
  } else {
    m_fboLocks[texture]++;
  }
}

void QOffscreenUi::releaseTexture(int texture) {
  Q_ASSERT(m_fboMap.count(texture));
  Q_ASSERT(m_fboLocks.count(texture));
  int newLockCount = --m_fboLocks[texture];
  if (!newLockCount) {
    m_readyFboQueue.push_back(m_fboMap[texture].data());
    m_fboLocks.remove(texture);
  }
}

QOpenGLFramebufferObject* QOffscreenUi::getReadyFbo() {
  QOpenGLFramebufferObject* result = nullptr;
  if (m_readyFboQueue.empty()) {
    qDebug() << "Building new offscreen FBO number " << m_fboMap.size() + 1;
    result = new QOpenGLFramebufferObject(QSize(m_uiSize.x, m_uiSize.y), 
        QOpenGLFramebufferObject::CombinedDepthStencil);
    m_fboMap[result->texture()] = QSharedPointer<QOpenGLFramebufferObject>(result);
    m_readyFboQueue.push_back(result);
  }
  return m_readyFboQueue.front();
}

void QOffscreenUi::updateQuick() {
  if (m_paused) {
    return;
  }
  if (!m_context->makeCurrent(m_offscreenSurface))
    return;

  // Polish, synchronize and render the next frame (into our fbo).  In this example
  // everything happens on the same thread and therefore all three steps are performed
  // in succession from here. In a threaded setup the render() call would happen on a
  // separate thread.
  if (m_polish) {
    m_renderControl->polishItems();
    m_renderControl->sync();
    m_polish = false;
  }

  QOpenGLFramebufferObject* fbo;
  
  {
    std::unique_lock<std::mutex> Lock(renderLock);
    fbo = getReadyFbo();
  }

  m_quickWindow->setRenderTarget(fbo);
  fbo->bind();
  m_renderControl->render();
  m_quickWindow->resetOpenGLState();
  QOpenGLFramebufferObject::bindDefault();
  glFinish();

  emit textureUpdated(fbo->texture());
}

QPointF QOffscreenUi::mapWindowToUi(const QPointF & p) {
    vec2 pos = vec2(p.x(), p.y());
    pos /= m_sourceSize;
    pos *= m_uiSize;
    return QPointF(pos.x, pos.y);
}


///////////////////////////////////////////////////////
//
// Event handling customization
// 
void QOffscreenUi::mouseMoved(vec2 mp) {
    // Interpret the mouse position as NDC coordinates
    mp /= m_sourceSize;
    mp *= 2.0f;
    mp -= 1.0f;
    mp.y *= -1.0f;
    mousePosition.store(QSizeF(mp.x, mp.y));
}

bool QOffscreenUi::interceptEvent(QEvent * e) {
    static bool dismissedHmd = false;
    switch (e->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
        if (QApplication::sendEvent(m_quickWindow, e)) {
            return true;
        }
    }
    break;

    case QEvent::Wheel:
    {
        QWheelEvent * we = (QWheelEvent*)e;
        QWheelEvent mappedEvent(mapWindowToUi(we->pos()), we->delta(), we->buttons(), we->modifiers(), we->orientation());
        QCoreApplication::sendEvent(m_quickWindow, &mappedEvent);
        return true;
    }
    break;

    case QEvent::MouseMove:
    {
        QMouseEvent * me = (QMouseEvent *)e;
        mouseMoved(oria::qt::toGlm(me->localPos()));
    }
    // Fall through
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    {
        QMouseEvent * me = (QMouseEvent *)e;
        QMouseEvent mappedEvent(e->type(), mapWindowToUi(me->localPos()), me->screenPos(), me->button(), me->buttons(), me->modifiers());
        QCoreApplication::sendEvent(m_quickWindow, &mappedEvent);
        return QObject::event(e);
    }

    default: break;
    }

    return false;
}


static ImagePtr loadImageWithAlpha(const std::vector<uint8_t> & data, bool flip) {
  using namespace oglplus;
#ifdef HAVE_OPENCV
  cv::Mat image = cv::imdecode(data, cv::IMREAD_UNCHANGED);
  if (flip) {
    cv::flip(image, image, 0);
  }
  ImagePtr result(new images::Image(image.cols, image.rows, 1, 4, image.data,
    PixelDataFormat::BGRA, PixelDataInternalFormat::RGBA8));
  return result;
#else
  std::stringstream stream(std::string((const char*)&data[0], data.size()));
  return ImagePtr(new images::PNGImage(stream));
#endif
}

TexturePtr loadCursor(Resource res) {
  using namespace oglplus;
  TexturePtr texture(new Texture());
  Context::Bound(TextureTarget::_2D, *texture)
    .MagFilter(TextureMagFilter::Linear)
    .MinFilter(TextureMinFilter::Linear);

  ImagePtr image = loadImageWithAlpha(Platform::getResourceByteVector(res), true);
  // FIXME detect alignment properly, test on both OpenCV and LibPNG
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  Texture::Storage2D(TextureTarget::_2D, 1, PixelDataInternalFormat::RGBA8, image->Width() * 2, image->Height() * 2);
  {
    size_t size = image->Width() * 2 * image->Height() * 2 * 4;
    uint8_t * empty = new uint8_t[size]; memset(empty, 0, size);
    images::Image blank(image->Width() * 2, image->Height() * 2, 1, 4, empty);
    Texture::SubImage2D(TextureTarget::_2D, blank, 0, 0);
  }
  Texture::SubImage2D(TextureTarget::_2D, *image, image->Width(), 0);
  DefaultTexture().Bind(TextureTarget::_2D);
  return texture;
}

//void QOffscreenUi::deleteOldTextures(const std::vector<GLuint> & oldTextures) {
//  if (!m_context->makeCurrent(m_offscreenSurface))
//    return;
//  m_context->functions()->glDeleteTextures(oldTextures.size(), &oldTextures[0]);
//}
