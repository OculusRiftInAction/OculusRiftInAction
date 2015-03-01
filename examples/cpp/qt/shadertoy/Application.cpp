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
#include "Application.h"
#include "Globals.h"
#include "MainWindow.h"


QSharedPointer<QFile> LOG_FILE;
QtMessageHandler ORIGINAL_MESSAGE_HANDLER;

const char * ORG_NAME = "Oculus Rift in Action";
const char * ORG_DOMAIN = "oculusriftinaction.com";
const char * APP_NAME = "ShadertoyVR";

ShadertoyApp::ShadertoyApp(int argc, char ** argv) : QApplication(argc, argv) {
    Q_INIT_RESOURCE(ShadertoyVR);
    QCoreApplication::setOrganizationName(ORG_NAME);
    QCoreApplication::setOrganizationDomain(ORG_DOMAIN);
    QCoreApplication::setApplicationName(APP_NAME);
    CONFIG_DIR = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    QString currentLogName = CONFIG_DIR.absoluteFilePath("ShadertoyVR.log");
    LOG_FILE = QSharedPointer<QFile>(new QFile(currentLogName));
    if (LOG_FILE->exists()) {
        QFile::rename(currentLogName,
            CONFIG_DIR.absoluteFilePath("ShadertoyVR_" +
            QDateTime::currentDateTime().toString("yyyy.dd.MM_hh.mm.ss") + ".log"));
    }
    if (!LOG_FILE->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "Could not open log file";
    }
    ORIGINAL_MESSAGE_HANDLER = qInstallMessageHandler(MessageOutput);

    mainWindow = new MainWindow();
    mainWindow->start();
    mainWindow->requestActivate();
}

void ShadertoyApp::destroyWindow() {
    mainWindow->stop();
    delete mainWindow;
}

void ShadertoyApp::MessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    ORIGINAL_MESSAGE_HANDLER(type, context, msg);
    QByteArray localMsg = msg.toLocal8Bit();
    QString now = QDateTime::currentDateTime().toString("yyyy.dd.MM_hh:mm:ss");
    switch (type) {
    case QtDebugMsg:
        LOG_FILE->write(QString().sprintf("%s Debug:    %s (%s:%u, %s)\n", now.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function).toLocal8Bit());
        break;
    case QtWarningMsg:
        LOG_FILE->write(QString().sprintf("%s Warning:  %s (%s:%u, %s)\n", now.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function).toLocal8Bit());
        break;
    case QtCriticalMsg:
        LOG_FILE->write(QString().sprintf("%s Critical: %s (%s:%u, %s)\n", now.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function).toLocal8Bit());
        break;
    case QtFatalMsg:
        LOG_FILE->write(QString().sprintf("%s Fatal:    %s (%s:%u, %s)\n", now.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function).toLocal8Bit());
        LOG_FILE->flush();
        abort();
    }
    LOG_FILE->flush();
}


ShadertoyApp::~ShadertoyApp() {
    qInstallMessageHandler(ORIGINAL_MESSAGE_HANDLER);
    LOG_FILE->close();
}

void ShadertoyApp::setupDesktopWindow() {
    desktopWindow.setLayout(new QFormLayout());
    QLabel * label = new QLabel("Your Oculus Rift is now active.  Please put on your headset.  Share and enjoy");
    desktopWindow.layout()->addWidget(label);
    desktopWindow.show();
}
