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
#include "Fetcher.h"
#include "Globals.h"

#include "ShadertoyConfig.h"

static const QString SHADERTOY_API_URL = "https://www.shadertoy.com/api/v1/shaders";
static const QString SHADERTOY_MEDIA_URL = "https://www.shadertoy.com/media/shaders/";

void Fetcher::fetchUrl(QUrl url, std::function<void(QByteArray)> f) {
  QNetworkRequest request(url);
  qDebug() << "Requesting url " << url;
  request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, "ShadertoyVR/1.0");
  ++currentNetworkRequests;
  QNetworkReply * netReply = qnam.get(request);
  connect(netReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
    this, [&, url](QNetworkReply::NetworkError code) {
    qWarning() << "Got error " << code << " fetching url " << url;
  });
  connect(netReply, &QNetworkReply::finished, this, [&, f, netReply, url] {
    --currentNetworkRequests;
    qDebug() << "Got response for url " << url;
    QByteArray replyBuffer = netReply->readAll();
    f(replyBuffer);
  });
}

void Fetcher::fetchFile(const QUrl & url, const QString & path) {
  fetchUrl(url, [&, path](const QByteArray & replyBuffer) {
    QFile outputFile(path);
    outputFile.open(QIODevice::WriteOnly);
    outputFile.write(replyBuffer);
    outputFile.close();
  });
}

void Fetcher::fetchNextShader() {
#ifdef SHADERTOY_API_KEY
  while (!shadersToFetch.empty() && currentNetworkRequests <= 4) {
    QString nextShaderId = shadersToFetch.front();
    shadersToFetch.pop_front();
    QString shaderFile = CONFIG_DIR.absoluteFilePath("shadertoy/" + nextShaderId + ".json");
    QString shaderPreviewFile = CONFIG_DIR.absoluteFilePath("shadertoy/" + nextShaderId + ".jpg");
    if (QFile(shaderFile).exists() && QFile(shaderPreviewFile).exists()) {
      continue;
    }

    if (!QFile(shaderFile).exists()) {
      qDebug() << "Fetching shader " << nextShaderId;
      QUrl url(SHADERTOY_API_URL + QString().sprintf("/%s?key=%s", nextShaderId.toLocal8Bit().constData(), SHADERTOY_API_KEY));
      fetchUrl(url, [&, shaderFile](const QByteArray & replyBuffer) {
        QFile outputFile(shaderFile);
        outputFile.open(QIODevice::WriteOnly);
        outputFile.write(replyBuffer);
        outputFile.close();
      });
    }

    if (!QFile(shaderPreviewFile).exists()) {
      fetchFile(QUrl(SHADERTOY_MEDIA_URL + nextShaderId + ".jpg"), shaderPreviewFile);
    }
  }

  if (shadersToFetch.isEmpty()) {
    timer.stop();
    return;
  }
#endif
}

Fetcher::Fetcher() {
  connect(&timer, &QTimer::timeout, this, [&] {
    fetchNextShader();
  });
  CONFIG_DIR.mkpath("shadertoy");
}

void Fetcher::fetchNetworkShaders() {
#ifdef SHADERTOY_API_KEY
  qDebug() << "Fetching shader list";
  QUrl url(SHADERTOY_API_URL + QString().sprintf("?key=%s", SHADERTOY_API_KEY));
  fetchUrl(url, [&](const QByteArray & replyBuffer) {
    QJsonDocument jsonResponse = QJsonDocument::fromJson(replyBuffer);
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray shaders = jsonObject["Results"].toArray();
    for (int i = 0; i < shaders.count(); ++i) {
      QString shaderId = shaders.at(i).toString();
      shadersToFetch.push_back(shaderId);
    }
    timer.start(1000);
  });
#endif
}
