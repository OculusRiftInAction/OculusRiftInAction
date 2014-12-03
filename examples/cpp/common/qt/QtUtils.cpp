#include "Common.h"
#include "QtUtils.h"

#include <QDomDocument>
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
  vec2 scaleFactor = qt::toGlm(size()) / qt::toGlm(filterTarget->size());
  vec2 sourceHit = qt::toGlm(event->localPos());
  QPointF targetHit = qt::pointFromGlm(sourceHit * scaleFactor);
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

