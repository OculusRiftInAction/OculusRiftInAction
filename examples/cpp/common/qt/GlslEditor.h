#pragma once

#include <QtWidgets>
#include <QOpenGLWidget>
#include <QPixmap>

class GlslHighlighter : public QSyntaxHighlighter {
  Q_OBJECT

public:
  GlslHighlighter(bool nightMode = true, QTextDocument *parent = 0);

protected:
  void highlightBlock(const QString &text);

private:
  struct HighlightingRule {
    QRegExp pattern;
    QTextCharFormat format;
  };
  QVector<HighlightingRule> highlightingRules;

  QTextCharFormat multiLineCommentFormat;
  QRegExp commentStartExpression;
  QRegExp commentEndExpression;
};


class GlslEditor : public QPlainTextEdit {
  Q_OBJECT

  class LineNumberArea : public QWidget {
  public:
    LineNumberArea(GlslEditor *editor) : QWidget(editor) {
      codeEditor = editor;
    }

    QSize sizeHint() const {
      return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

  protected:
    void paintEvent(QPaintEvent *event) {
      codeEditor->lineNumberAreaPaintEvent(event);
    }

  private:
    GlslEditor *codeEditor;
  };

public:
  GlslEditor(QWidget *parent = 0);

  void lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int)blockBoundingRect(block).height();
    while (block.isValid() && top <= event->rect().bottom()) {
      if (block.isVisible() && bottom >= event->rect().top()) {
        QString number = QString::number(blockNumber + 1);
        painter.setPen(Qt::black);
        painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
          Qt::AlignRight, number);
      }

      block = block.next();
      top = bottom;
      bottom = top + (int)blockBoundingRect(block).height();
      ++blockNumber;
    }
  }

  int lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
      max /= 10;
      ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
  }

protected:
  void resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
  }

  private slots:
  void updateLineNumberAreaWidth(int newBlockCount) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
  }

  void highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
      QTextEdit::ExtraSelection selection;

      QColor lineColor = QColor(Qt::yellow).lighter(160);

      selection.format.setBackground(lineColor);
      selection.format.setProperty(QTextFormat::FullWidthSelection, true);
      selection.cursor = textCursor();
      selection.cursor.clearSelection();
      extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
  }

  void updateLineNumberArea(const QRect & rect, int dy) {
    if (dy)
      lineNumberArea->scroll(0, dy);
    else
      lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
      updateLineNumberAreaWidth(0);
  }

private:
  QWidget *lineNumberArea;
  GlslHighlighter highlighter;
};
