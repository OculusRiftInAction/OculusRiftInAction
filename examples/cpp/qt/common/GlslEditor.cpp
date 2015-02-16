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

  return finalMap;
}
//color0,
//  color1,
//  black,
//  white,
//  darkGray,
//  gray,
//  lightGray,
//  red,
//  green,
//  blue,
//  cyan,
//  magenta,
//  yellow,
//  darkRed,
//  darkGreen,
//  darkBlue,
//  darkCyan,
//  darkMagenta,
//  darkYellow,
//  transparent
//! [0]
GlslHighlighter::GlslHighlighter(bool nightMode, QTextDocument *parent)
  : QSyntaxHighlighter(parent) {

  multiLineCommentFormat.setForeground(nightMode ? Qt::gray : Qt::darkGray);

  Map m = createGlslMap();
  {
    HighlightingRule rule;
    rule.format.setForeground(nightMode ? Qt::yellow : Qt::darkBlue);
    rule.format.setFontWeight(QFont::Bold);
    foreach(const QString &keyword, m["Keyword"]) {
      rule.pattern = QRegExp("\\b" + keyword + "\\b");
      highlightingRules.append(rule);
    }
  }

  {
    HighlightingRule rule;
    rule.format.setForeground(nightMode ? Qt::green : Qt::darkGreen);
    rule.format.setFontWeight(QFont::Bold);
    foreach(const QString &keyword, m["Keywordstypes"]) {
      rule.pattern = QRegExp("\\b" + keyword + "\\b");
      highlightingRules.append(rule);
    }
  }

  // Buildconstants
  // Buildinvariables
  // Reserved


  {
    HighlightingRule rule;
    QTextCharFormat functionFormat;
    rule.format.setFontItalic(true);
    rule.format.setForeground(nightMode ? Qt::yellow : Qt::blue);
    rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
    highlightingRules.append(rule);
  }

  {
    HighlightingRule rule;
    rule.format.setForeground(nightMode ? Qt::yellow : Qt::darkBlue);
    rule.format.setFontWeight(QFont::Bold);
    rule.format.setFontItalic(true);
    foreach(const QString &keyword, m["Buildinfunctions"]) {
      //      rule.pattern = QRegExp("\\b" + keyword + "\\b");
      rule.pattern = QRegExp("\\b" + keyword + "(?=\\()");
      highlightingRules.append(rule);
    }
  }

  {
    HighlightingRule rule;
    rule.format.setForeground(nightMode ? Qt::yellow : Qt::darkBlue);
    rule.format.setFontWeight(QFont::Bold);
    rule.format.setFontItalic(true);
    foreach(const QString &keyword, m["Reserved"]) {
      rule.pattern = QRegExp("\\b" + keyword + "\\b");
      highlightingRules.append(rule);
    }
  }

  {
    HighlightingRule rule;
    QTextCharFormat singleLineCommentFormat;
    singleLineCommentFormat.setForeground(nightMode ? Qt::gray  : Qt::darkGray);
    rule.pattern = QRegExp("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
  }

  {
    HighlightingRule rule;
    rule.format.setForeground(nightMode ? Qt::green : Qt::darkGreen);
    rule.pattern = QRegExp("\".*\"");
    highlightingRules.append(rule);
  }


  commentStartExpression = QRegExp("/\\*");
  commentEndExpression = QRegExp("\\*/");
}

void GlslHighlighter::highlightBlock(const QString &text) {
  foreach(const HighlightingRule &rule, highlightingRules) {
    QRegExp expression(rule.pattern);
    int index = expression.indexIn(text);
    while (index >= 0) {
      int length = expression.matchedLength();
      setFormat(index, length, rule.format);
      index = expression.indexIn(text, index + length);
    }
  }
  //! [7] //! [8]
  setCurrentBlockState(0);
  //! [8]

  //! [9]
  int startIndex = 0;
  if (previousBlockState() != 1)
    startIndex = commentStartExpression.indexIn(text);

  //! [9] //! [10]
  while (startIndex >= 0) {
    //! [10] //! [11]
    int endIndex = commentEndExpression.indexIn(text, startIndex);
    int commentLength;
    if (endIndex == -1) {
      setCurrentBlockState(1);
      commentLength = text.length() - startIndex;
    } else {
      commentLength = endIndex - startIndex
        + commentEndExpression.matchedLength();
    }
    setFormat(startIndex, commentLength, multiLineCommentFormat);
    startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
  }
}
//! [11]

GlslEditor::GlslEditor(QWidget *parent) {
   setFont(QFont("Courier", 12));
   highlighter.setDocument(document());
   lineNumberArea = new LineNumberArea(this);

   connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
   connect(this, SIGNAL(updateRequest(QRect, int)), this, SLOT(updateLineNumberArea(QRect, int)));
   connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

   updateLineNumberAreaWidth(0);
   highlightCurrentLine();
}

#include "moc_GlslEditor.cpp"
