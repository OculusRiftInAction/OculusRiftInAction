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
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOffscreenSurface>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickImageProvider>


namespace oria { namespace qt {
  vec2 toGlm(const QSize & size);
  vec2 toGlm(const QPointF & pt);
  QSize sizeFromGlm(const vec2 & size);
  QPointF pointFromGlm(const vec2 & pt);
  QByteArray toByteArray(Resource res);
  QString toString(Resource res);
  QImage loadImageResource(Resource res);
  QPixmap loadXpmResource(Resource res);

} } // namespaces


inline QByteArray readFileToByteArray(const QString & fileName) {
  QFile f(fileName);
  f.open(QFile::ReadOnly);
  return f.readAll();
}

inline std::vector<uint8_t> readFileToVector(const QString & fileName) {
  QByteArray ba = readFileToByteArray(fileName);
  return std::vector<uint8_t>(ba.constData(), ba.constData() + ba.size());
}

inline QString readFileToString(const QString & fileName) {
  return QString(readFileToByteArray(fileName));
}


/**
 * Forwards mouse and keyboard input from the specified widget to the
 * graphics view, allowing the user to click on one widget (like an
 * OpenGl window with controls rendered inside it) and have the resulting
 * click reflected on the scene displayed in this view.
 */

class QResourceImageProvider : public QQuickImageProvider {
  std::map<QString, Resource> map;
public:
  QResourceImageProvider() : QQuickImageProvider(QQmlImageProviderBase::Image) {
    for (int i = 0; Resources::RESOURCE_MAP_VALUES[i].first != NO_RESOURCE; ++i) {
      const Resources::Pair & r = Resources::RESOURCE_MAP_VALUES[i];
      map[r.second.c_str()] = r.first;
    }
  }

  virtual QImage	requestImage(const QString & id, QSize * size, const QSize & requestedSize) {
    qDebug() << "Requested image " << id << " " << requestedSize;
    QImage image;

    if (map.count(id)) {
      if (size) {
        *size = QSize(128, 128);
      }
      image.loadFromData(oria::qt::toByteArray(map[id]));
      image = image.scaled(QSize(128, 128));
    }

    return image;
  }
};

class QMyQuickRenderControl : public QQuickRenderControl {
public:
  QWindow * m_renderWindow{ nullptr };

  QWindow * renderWindow(QPoint * offset) Q_DECL_OVERRIDE{
    if (nullptr == m_renderWindow) {
      return QQuickRenderControl::renderWindow(offset);
    }
    if (nullptr != offset) {
      offset->rx() = offset->ry() = 0;
    }
    return m_renderWindow;
  }

};


class QOffscreenUi : public QObject {
  Q_OBJECT

  bool m_paused;
public:
  QOffscreenUi();
  ~QOffscreenUi();
  void setup(const QSize & size, QOpenGLContext * context);
  void loadQml(const QUrl & qmlSource, std::function<void(QQmlContext*)> f = [](QQmlContext*){});
  QQmlContext * qmlContext();

  void pause() {
    m_paused = true;
  }

  void resume() {
    m_paused = false;
    requestRender();
  }

  void setProxyWindow(QWindow * window) {
    m_renderControl->m_renderWindow = window;
  }

private slots:
  void updateQuick();
  void run();

public slots:
  void requestUpdate();
  void requestRender();
  void lockTexture(int texture);
  void releaseTexture(int texture);

signals:
  void textureUpdated(int texture);

private:
  QMap<int, QSharedPointer<QOpenGLFramebufferObject>> m_fboMap;
  QMap<int, int> m_fboLocks;
  QQueue<QOpenGLFramebufferObject*> m_readyFboQueue;
  
  QOpenGLFramebufferObject* getReadyFbo();

public:
  QOpenGLContext *m_context{ new QOpenGLContext };
  QOffscreenSurface *m_offscreenSurface{ new QOffscreenSurface };
  QMyQuickRenderControl  *m_renderControl{ new QMyQuickRenderControl };
  QQuickWindow *m_quickWindow{ nullptr };
  QQmlEngine *m_qmlEngine{ nullptr };
  QQmlComponent *m_qmlComponent{ nullptr };
  QQuickItem * m_rootItem{ nullptr };
  QTimer m_updateTimer;
  QSize m_size;
  bool m_polish{ true };
  std::mutex renderLock;
};

class LambdaThread : public QThread {
  Q_OBJECT
  Lambda f;

  void run() {
    f();
  }

public:
  LambdaThread() {}

  template <typename F>
  LambdaThread(F f) : f(f) {}

  template <typename F>
  void setLambda(F f) { this->f = f; }
};


#ifdef OS_WIN
#define QT_APP_WITH_ARGS(AppClass) \
  int argc = 1; \
  char ** argv = &lpCmdLine;  \
  AppClass app(argc, argv);
#else
#define QT_APP_WITH_ARGS(AppClass) AppClass app(argc, argv);
#endif

#define RUN_QT_APP(AppClass) \
MAIN_DECL { \
  try { \
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", "."); \
    QT_APP_WITH_ARGS(AppClass); \
    return app.exec(); \
  } catch (std::exception & error) { \
    SAY_ERR(error.what()); \
  } catch (const std::string & error) { \
    SAY_ERR(error.c_str()); \
  } \
  return -1; \
}


