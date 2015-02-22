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

#pragma once

#include <QNetworkAccessManager>
#include <QtNetwork>

class Fetcher : public QObject {
  Q_OBJECT
public:
  Fetcher();
  void fetchNetworkShaders();
private:
  QQueue<QString> shadersToFetch;
  QNetworkAccessManager qnam;
  QTimer timer;
  int currentNetworkRequests{ 0 };

  virtual void fetchUrl(QUrl url, std::function<void(QByteArray)> f);
  void fetchFile(const QUrl & url, const QString & path);
  void fetchNextShader();
};
