#include "Common.h"

#ifdef HAVE_QT

//#include <QtWidgets/QGraphicsView>

namespace qt {

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
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
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

#endif
