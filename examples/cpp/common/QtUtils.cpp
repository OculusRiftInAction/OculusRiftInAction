#include "Common.h"

#ifdef HAVE_QT

//#include <QtWidgets/QGraphicsView>

ForwardingGraphicsView::ForwardingGraphicsView(QWidget * filterTarget) : filterTarget(filterTarget) {
  filterTarget->installEventFilter(this);
  filterTarget->setMouseTracking(true);
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
  // Dispatch
  viewportEvent(&newEvent);
}

bool ForwardingGraphicsView::eventFilter(QObject *object, QEvent *event) {
  if (object == filterTarget) {
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
      forwardMouseEvent((QMouseEvent *)event);
      break;
    default:
      break;
    }
    return false;
  }
  return QGraphicsView::eventFilter(object, event);
}

#endif
