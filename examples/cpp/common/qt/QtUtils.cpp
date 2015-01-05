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

#include "Common.h"
#include "Qt/QtUtils.h"

#include <QDomDocument>


namespace oria {
  namespace qt {

    template<typename T>
    T toQtType(Resource res) {
      T result;
      size_t size = Resources::getResourceSize(res);
      result.resize(size);
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

    QImage loadImageResource(Resource res) {
      QImage image;
      image.loadFromData(toByteArray(res));
      return image;
    }

    QPixmap loadXpmResource(Resource res) {
      QString cursorXpmStr = oria::qt::toString(res);
      QStringList list = cursorXpmStr.split(QRegExp("\\n|\\r\\n|\\r"));
      std::vector<QByteArray> bv;
      std::vector<const char*> v;
      foreach(QString line, list) {
        bv.push_back(line.toLocal8Bit());
        v.push_back(*bv.rbegin());
      }
      QPixmap result = QPixmap(&v[0]);
      return result;
    }
  }
}
ForwardingGraphicsView::ForwardingGraphicsView(QWidget * filterTarget)  {
  if (filterTarget) {
    install(filterTarget);
  }
}

void ForwardingGraphicsView::install(QWidget * filterTarget) {
  if (filterTarget == this->filterTarget) {
    return;
  }
  if (this->filterTarget) {
    remove(this->filterTarget);
  }

  this->filterTarget = filterTarget;

  if (this->filterTarget) {
    this->filterTarget->installEventFilter(this);
  }
}

void ForwardingGraphicsView::remove(QWidget * filterTarget) {
  if (!filterTarget || filterTarget != this->filterTarget) {
    return;
  }

  if (filterTarget) {
    filterTarget->removeEventFilter(this);
    this->filterTarget = nullptr;
  }
}

void ForwardingGraphicsView::resizeEvent(QResizeEvent *event) {
  if (scene())
    scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
  QGraphicsView::resizeEvent(event);
}

void ForwardingGraphicsView::forwardMouseEvent(QMouseEvent * event) {
  // Translate the incoming coordinates to the target window coordinates
  vec2 scaleFactor = oria::qt::toGlm(size()) / oria::qt::toGlm(filterTarget->size());
  vec2 sourceHit = oria::qt::toGlm(event->localPos());
  QPointF targetHit = oria::qt::pointFromGlm(sourceHit * scaleFactor);
  QPointF screenHit = mapToGlobal(targetHit.toPoint());
  
  // Build a new event
  QMouseEvent newEvent(event->type(),
    targetHit, targetHit, screenHit,
    event->button(), event->buttons(), event->modifiers());
  viewportEvent(&newEvent);
}

void ForwardingGraphicsView::forwardKeyEvent(QKeyEvent * event) {
  viewportEvent(event);
}

bool ForwardingGraphicsView::eventFilter(QObject *object, QEvent *event) {
  if (object == filterTarget) {
    switch (event->type()) {
    case QEvent::MouseMove:
      forwardMouseEvent((QMouseEvent *)event);
      break;

    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
      forwardMouseEvent((QMouseEvent *)event);
      break;

    case QEvent::Wheel:
      viewportEvent((QWheelEvent *)event);
      break;

    case QEvent::KeyPress:
      keyPressEvent((QKeyEvent*)event);
      break;

    case QEvent::KeyRelease:
      keyReleaseEvent((QKeyEvent*)event);
      break;

    default:
      break;
    }
    return false;
  }
  return QGraphicsView::eventFilter(object, event);
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

OffscreenUiWindow::OffscreenUiWindow() {
  // Create a QQuickWindow that is associated with out render control. Note that this
  // window never gets created or shown, meaning that it will never get an underlying
  // native (platform) window.
  m_quickWindow = new QQuickWindow(this);

  // When Quick says there is a need to render, we will not render immediately. Instead,
  // a timer with a small interval is used to get better performance.
  //m_updateTimer.setSingleShot(true);
  //m_updateTimer.setInterval(5);
  //connect(&m_updateTimer, &QTimer::timeout, this, &OffscreenUiWindow::updateQuick);

  // Now hook up the signals. For simplicy we don't differentiate between
  // renderRequested (only render is needed, no sync) and sceneChanged (polish and sync
  // is needed too).
  connect(this, &QQuickRenderControl::renderRequested, this, [&] {
    qDebug() << "Render only " << Platform::elapsedMillis();
    renderSceneToFbo();
    qDebug() << "Render only done" << Platform::elapsedMillis();
  });
  connect(this, &QQuickRenderControl::sceneChanged, this, [&] {
    if (!m_fbo) {
      return;
    }
    // Polish, synchronize and render the next frame (into our fbo).  In this example
    // everything happens on the same thread and therefore all three steps are performed
    // in succession from here. In a threaded setup the render() call would happen on a
    // separate thread.
    static bool inUpdate = false;
    if (inUpdate) {
      return;
    }
    qDebug() << "Update " << Platform::elapsedMillis();

    inUpdate = true;
    polishItems();
    sync();
    renderSceneToFbo();
    inUpdate = false;

    qDebug() << "Update done " << Platform::elapsedMillis();
  });
}



void OffscreenUiWindow::setupScene(const QSize & size, QOpenGLContext * context, QQmlComponent* qmlComponent) {
  m_qmlComponent = qmlComponent;
  if (m_qmlComponent->isError()) {
    QList<QQmlError> errorList = m_qmlComponent->errors();
    foreach(const QQmlError &error, errorList)
      qWarning() << error.url() << error.line() << error;
  }

  QObject *rootObject = m_qmlComponent->create();
  if (m_qmlComponent->isError()) {
    QList<QQmlError> errorList = m_qmlComponent->errors();
    foreach(const QQmlError &error, errorList)
      qWarning() << error.url() << error.line() << error;
    return;
  }

  QQuickItem *m_rootItem{ nullptr };
  m_rootItem = qobject_cast<QQuickItem *>(rootObject);
  if (!m_rootItem) {
    qWarning("run: Not a QQuickItem");
    delete rootObject;
    return;
  }

  // The root item is ready. Associate it with the window.
  m_rootItem->setParentItem(m_quickWindow->contentItem());

  // Update item and rendering related geometries.
  m_quickWindow->setGeometry(0, 0, size.width(), size.height());

  // Initialize the render control and our OpenGL resources.
  initialize(context);

  m_fbo = new QOpenGLFramebufferObject(size, QOpenGLFramebufferObject::CombinedDepthStencil);
  m_quickWindow->setRenderTarget(m_fbo);
  emit sceneChanged();
}

void OffscreenUiWindow::renderSceneToFbo() {
  if (!m_fbo) {
    return;
  }
  m_fbo->bind();
  render();
  m_quickWindow->resetOpenGLState();
  QOpenGLFramebufferObject::bindDefault();
  glFlush();
  GLuint oldTexture = m_currentTexture.exchange(m_fbo->takeTexture());
  if (oldTexture) {
    glDeleteTextures(1, &oldTexture);
  }
  emit offscreenTextureUpdated();
}
