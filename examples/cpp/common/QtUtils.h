#pragma once

#ifdef HAVE_QT

//#include <QtWidgets/QGraphicsView>
#include <QtWidgets>
#include <QGLWidget>
#include <QPixmap>

namespace qt {
  inline vec2 toGlm(const QSize & size) {
    return vec2(size.width(), size.height());
  }

  inline vec2 toGlm(const QPointF & pt) {
    return vec2(pt.x(), pt.y());
  }

  inline QSize sizeFromGlm(const vec2 & size) {
    return QSize(size.x, size.y);
  }

  inline QPointF pointFromGlm(const vec2 & pt) {
    return QPointF(pt.x, pt.y);
  }
}
/**
 * Forwards mouse and keyboard input from the specified widget to the
 * graphics view, allowing the user to click on one widget (like an
 * OpenGl window with controls rendered inside it) and have the resulting
 * click reflected on the scene displayed in this view.
 */
class ForwardingGraphicsView : public QGraphicsView {
  QWidget * filterTarget;

public:
  ForwardingGraphicsView(QWidget * filterTarget);

protected:
  void forwardMouseEvent(QMouseEvent * event);
  void resizeEvent(QResizeEvent *event);
  bool eventFilter(QObject *object, QEvent *event);
};


class PaintlessGlWidget : public QGLWidget {
protected:
  bool event(QEvent * e) {
    if (QEvent::Paint == e->type()) {
      return true;
    }
    return QGLWidget::event(e);
  }

public:
  explicit PaintlessGlWidget(const QGLFormat& format) : QGLWidget(format) {
    setAutoBufferSwap(false);
  }
};

#endif

