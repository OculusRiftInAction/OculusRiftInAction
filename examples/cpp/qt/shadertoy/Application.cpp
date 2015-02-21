#include "QtCommon.h"
#include "Application.h"
#include "Globals.h"

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
#if (!defined(_DEBUG) && defined(TRACKERBIRD_PRODUCT_ID))
    QCoreApplication::setApplicationVersion(QString::fromWCharArray(TRACKERBIRD_PRODUCT_VERSION));
#endif
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
    ORIGINAL_MESSAGE_HANDLER = qInstallMessageHandler(myMessageOutput);
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
