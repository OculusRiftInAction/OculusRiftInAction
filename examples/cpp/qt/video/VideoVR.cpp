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

#ifdef _DEBUG
#define BRAD_DEBUG 1
#endif

#include "QtCommon.h"

#include <QQuickTextDocument>
#include <QGuiApplication>
#include <QQmlContext>
#include <QNetworkAccessManager>
#include <QtNetwork>
#include <QDeclarativeEngine>

const char * ORG_NAME = "Oculus Rift in Action";
const char * ORG_DOMAIN = "oculusriftinaction.com";
const char * APP_NAME = "VideoVR";

QDir CONFIG_DIR;
QSharedPointer<QFile> LOG_FILE;
QtMessageHandler ORIGINAL_MESSAGE_HANDLER;

using namespace oglplus;

static uvec2 UI_SIZE(1280, 720);
static float UI_ASPECT = aspect(vec2(UI_SIZE));
static float UI_INVERSE_ASPECT = 1.0f / UI_ASPECT;

class MainWindow : public QRiftWindow {
    Q_OBJECT
    using AtomicGlTexture = std::atomic<GLuint>;
    using SyncPair = std::pair<GLuint, GLsync>;
    using TextureTrashcan = std::queue<SyncPair>;
    using TextureDeleteQueue = std::vector<GLuint>;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    // A cache of all the input textures available 
    QDir configPath;
    QSettings settings;

    // The shadertoy rendering resolution scale.  1.0 means full resolution 
    // as defined by the Oculus SDK as the ideal offscreen resolution 
    // pre-distortion
    float texRes{ 1.0f };

    //////////////////////////////////////////////////////////////////////////////
    // 
    // Offscreen UI
    //
    QOffscreenUi * uiWindow{ new QOffscreenUi() };
    vec2 windowSize;

    //////////////////////////////////////////////////////////////////////////////
    // 
    // Shader Rendering information
    //

    // We actually render the shader to one FBO for dynamic framebuffer scaling,
    // while leaving the actual texture we pass to the Oculus SDK fixed.
    // This allows us to have a clear UI regardless of the shader performance
    FramebufferWrapperPtr vrFramebuffer;

    // The current mouse position as reported by the main thread
    bool uiVisible{ false };
    QVariantAnimation animation;
    float animationValue;

    // A wrapper for passing the UI texture from the app to the widget
    AtomicGlTexture uiTexture{ 0 };
    TextureTrashcan textureTrash;
    TextureDeleteQueue textureDeleteQueue;
    Mutex textureLock;
    QTimer timer;

    // GLSL and geometry for the UI
    ProgramPtr uiProgram;
    ShapeWrapperPtr uiShape;
    TexturePtr mouseTexture;
    ShapeWrapperPtr mouseShape;

    // For easy compositing the UI texture and the mouse texture
    FramebufferWrapperPtr uiFramebuffer;

    // Geometry and shader for rendering the possibly low res shader to the main framebuffer
    ProgramPtr planeProgram;
    ShapeWrapperPtr plane;

    // Measure the FPS for use in dynamic scaling
    GLuint exchangeUiTexture(GLuint newUiTexture) {
        return uiTexture.exchange(newUiTexture);
    }

    vec2 textureSize() {
#ifdef USE_RIFT
        return vec2(ovr::toGlm(eyeTextures[0].Header.TextureSize));
#else
        return vec2(size().width(), size().height());
#endif
    }

    uvec2 renderSize() {
        return uvec2(texRes * textureSize());
    }

public:
    MainWindow() {
        // Fixes an occasional crash caused by a race condition between the Rift 
        // render thread and the UI thread, triggered when Rift swapbuffers overlaps 
        // with the UI thread binding a new FBO (specifically, generating a texture 
        // for the FBO.  
        // Perhaps I should just create N FBOs and have the UI object iterate over them
        {
            QString configLocation = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
            configPath = QDir(configLocation);
            configPath.mkpath("shaders");
        }

        connect(&timer, &QTimer::timeout, this, &MainWindow::onTimer);
        timer.start(100);
        setupOffscreenUi();
        Platform::addShutdownHook([&] {
            vrFramebuffer.reset();
            uiProgram.reset();
            uiShape.reset();
            uiFramebuffer.reset();
            mouseTexture.reset();
            mouseShape.reset();
            uiFramebuffer.reset();
            planeProgram.reset();
            plane.reset();
        });
    }

    virtual void stop() {
        delete uiWindow;
        uiWindow = nullptr;
    }

private:
    virtual void setup() {
        QRiftWindow::setup();
        // The geometry and shader for rendering the 2D UI surface when needed
        uiProgram = oria::loadProgram(
            Resource::SHADERS_TEXTURED_VS,
            Resource::SHADERS_TEXTURED_FS);
        uiShape = oria::loadPlane(uiProgram, UI_ASPECT);

        // The geometry and shader for scaling up the rendered shadertoy effect
        // up to the full offscreen render resolution.  This is then compositied 
        // with the UI window
        planeProgram = oria::loadProgram(
            Resource::SHADERS_TEXTURED_VS,
            Resource::SHADERS_TEXTURED_FS);
        plane = oria::loadPlane(planeProgram, 1.0);

        mouseTexture = loadCursor(Resource::IMAGES_CURSOR_PNG);
        mouseShape = oria::loadPlane(uiProgram, UI_INVERSE_ASPECT);

        uiFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
        uiFramebuffer->init(UI_SIZE);

        vrFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
        vrFramebuffer->init(textureSize());

        DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
    }

    void setupOffscreenUi() {
#ifdef USE_RIFT
        this->endFrameLock = &uiWindow->renderLock;
#endif
        qApp->setFont(QFont("Arial", 14, QFont::Bold));
        uiWindow->pause();
        uiWindow->setup(QSize(UI_SIZE.x, UI_SIZE.y), context());
        uiWindow->setProxyWindow(this);
        {
            auto qmlContext = uiWindow->m_qmlEngine->rootContext();
        }

        QUrl qml = QUrl("qrc:/layouts/Main.qml");
        uiWindow->m_qmlEngine->addImportPath("./qml");
        uiWindow->m_qmlEngine->addImportPath("./layouts");
        uiWindow->m_qmlEngine->addImportPath(".");
        uiWindow->loadQml(qml);
        connect(uiWindow, &QOffscreenUi::textureUpdated, this, [&](int textureId) {
            uiWindow->lockTexture(textureId);
            GLuint oldTexture = exchangeUiTexture(textureId);
            if (oldTexture) {
                uiWindow->releaseTexture(oldTexture);
            }
        });

        QQuickItem * editorControl;
        editorControl = uiWindow->m_rootItem->findChild<QQuickItem*>("shaderTextEdit");
        if (editorControl) {
            qDebug() << "Found editor window";
        }

        QObject::connect(uiWindow->m_rootItem, SIGNAL(toggleUi()),
            this, SLOT(onToggleUi()));
        QObject::connect(uiWindow->m_rootItem, SIGNAL(recenterPose()),
            this, SLOT(onRecenterPosition()));
        QObject::connect(uiWindow->m_rootItem, SIGNAL(modifyTextureResolution(double)),
            this, SLOT(onModifyTextureResolution(double)));
        QObject::connect(uiWindow->m_rootItem, SIGNAL(startShutdown()),
            this, SLOT(onShutdown()));
        QObject::connect(&animation, &QVariantAnimation::valueChanged, this, [&](const QVariant & val) {
            animationValue = val.toFloat();
        });

        connect(this, &MainWindow::fpsUpdated, this, [&](float fps) {
            setItemText("fps", QString().sprintf("%0.0f", fps));
        });

        setItemText("res", QString().sprintf("%0.2f", texRes));
        Platform::addShutdownHook([&] {
        });
    }

    QVariant getItemProperty(const QString & itemName, const QString & property) {
        QQuickItem * item = uiWindow->m_rootItem->findChild<QQuickItem*>(itemName);
        if (nullptr != item) {
            return item->property(property.toLocal8Bit());
        } else {
            qWarning() << "Could not find item " << itemName << " on which to set property " << property;
        }
        return QVariant();
    }

    void setItemProperty(const QString & itemName, const QString & property, const QVariant & value) {
        QQuickItem * item = uiWindow->m_rootItem->findChild<QQuickItem*>(itemName);
        if (nullptr != item) {
            bool result = item->setProperty(property.toLocal8Bit(), value);
            if (!result) {
                qWarning() << "Set property " << property << " on item " << itemName << " returned " << result;
            }
        } else {
            qWarning() << "Could not find item " << itemName << " on which to set property " << property;
        }
    }

    void setItemText(const QString & itemName, const QString & text) {
        setItemProperty(itemName, "text", text);
    }

    QString getItemText(const QString & itemName) {
        return getItemProperty(itemName, "text").toString();
    }

private slots:
    void onToggleUi() {
        uiVisible = !uiVisible;
        setItemProperty("shaderTextEdit", "readOnly", !uiVisible);
        if (uiVisible) {
            animation.stop();
            animation.setStartValue(0.0);
            animation.setEndValue(1.0);
            animation.setEasingCurve(QEasingCurve::OutBack);
            animation.setDuration(500);
            animation.start();
            uiWindow->resume();
        } else {
            animation.stop();
            animation.setStartValue(1.0);
            animation.setEndValue(0.0);
            animation.setEasingCurve(QEasingCurve::InBack);
            animation.setDuration(500);
            animation.start();
            uiWindow->pause();
        }
    }

    void onRecenterPosition() {
#ifdef USE_RIFT
        queueRenderThreadTask([&] {
            ovrHmd_RecenterPose(hmd);
});
#endif
  }

    void onModifyTextureResolution(double scale) {
        float newRes = scale * texRes;
        newRes = std::max(0.1f, std::min(1.0f, newRes));
        if (newRes != texRes) {
            queueRenderThreadTask([&, newRes] {
                texRes = newRes;
            });
            setItemText("res", QString().sprintf("%0.2f", newRes));
        }
    }

    void onShutdown() {
        QApplication::instance()->quit();
    }

    void onTimer() {
        TextureDeleteQueue tempTextureDeleteQueue;

        // Scope the lock tightly
        {
            Lock lock(textureLock);
            if (!textureDeleteQueue.empty()) {
                tempTextureDeleteQueue.swap(textureDeleteQueue);
            }
        }

        if (!tempTextureDeleteQueue.empty()) {
            std::for_each(tempTextureDeleteQueue.begin(), tempTextureDeleteQueue.end(), [&](int usedTexture) {
                uiWindow->releaseTexture(usedTexture);
            });
        }
    }

private:

    ///////////////////////////////////////////////////////
    //
    // Event handling customization
    // 
    void mouseMoveEvent(QMouseEvent * me) {
        // Make sure we don't show the system cursor over the window
        qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
        QRiftWindow::mouseMoveEvent(me);
    }

    bool event(QEvent * e) {
#ifdef USE_RIFT
        static bool dismissedHmd = false;
        switch (e->type()) {
        case QEvent::KeyPress:
            if (!dismissedHmd) {
                // Allow the user to remove the HSW message early
                ovrHSWDisplayState hswState;
                ovrHmd_GetHSWDisplayState(hmd, &hswState);
                if (hswState.Displayed) {
                    ovrHmd_DismissHSWDisplay(hmd);
                    dismissedHmd = true;
                    return true;
                }
        }
    }
#endif
        if (uiWindow) {
            if (uiWindow->interceptEvent(e)) {
                return true;
            }
        }

        return QRiftWindow::event(e);
  }

    void resizeEvent(QResizeEvent *e) {
        windowSize = vec2(e->size().width(), e->size().height());
        uiWindow->setSourceSize(e->size());
    }

    ///////////////////////////////////////////////////////
    //
    // Rendering functionality
    // 
    void perFrameRender() {
        Context::Enable(Capability::Blend);
        Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
        Context::Disable(Capability::ScissorTest);
        Context::Disable(Capability::DepthTest);
        Context::Disable(Capability::CullFace);
        if (uiVisible) {
            static GLuint lastUiTexture = 0;
            static GLsync lastUiSync;
            GLuint currentUiTexture = uiTexture.exchange(0);
            if (0 == currentUiTexture) {
                currentUiTexture = lastUiTexture;
            } else {
                // If the texture has changed, push it into the trash bin for
                // deletion once it's finished rendering
                if (lastUiTexture) {
                    textureTrash.push(SyncPair(lastUiTexture, lastUiSync));
                }
                lastUiTexture = currentUiTexture;
            }
            MatrixStack & mv = Stacks::modelview();
            if (currentUiTexture) {
                Texture::Active(0);
                // Composite the UI image and the mouse sprite
                uiFramebuffer->Bound([&] {
                    Context::Clear().ColorBuffer();
                    oria::viewport(UI_SIZE);
                    // Clear out the projection and modelview here.
                    Stacks::withIdentity([&] {
                        glBindTexture(GL_TEXTURE_2D, currentUiTexture);
                        oria::renderGeometry(plane, uiProgram);

                        // Render the mouse sprite on the UI
                        QSizeF size = uiWindow->getMousePosition().load();
                        vec2 mp(size.width(), size.height());
                        mv.translate(vec3(mp, 0.0f));
                        mv.scale(vec3(0.1f));
                        mouseTexture->Bind(Texture::Target::_2D);
                        oria::renderGeometry(mouseShape, uiProgram);
                    });
                });
                lastUiSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            }
        }

        TextureDeleteQueue tempTextureDeleteQueue;
        while (!textureTrash.empty()) {
            SyncPair & top = textureTrash.front();
            GLuint & texture = top.first;
            GLsync & sync = top.second;
            GLenum result = glClientWaitSync(sync, 0, 0);
            if (GL_ALREADY_SIGNALED == result || GL_CONDITION_SATISFIED == result) {
                tempTextureDeleteQueue.push_back(texture);
                textureTrash.pop();
            } else {
                break;
            }
        }

        if (!tempTextureDeleteQueue.empty()) {
            Lock lock(textureLock);
            textureDeleteQueue.insert(textureDeleteQueue.end(),
                tempTextureDeleteQueue.begin(), tempTextureDeleteQueue.end());
        }
    }

    void perEyeRender() {
        // Render the shadertoy effect into a framebuffer, possibly at a 
        // smaller resolution than recommended
        vrFramebuffer->Bound([&] {
            glClearColor(0.5f, 0.5f, 0.5f, 1);
            Context::Clear().ColorBuffer();
            oria::viewport(renderSize());
        });
        oria::viewport(textureSize());

        // Now re-render the shader output to the screen.
        vrFramebuffer->BindColor(Texture::Target::_2D);
#ifdef USE_RIFT
        if (true) {
#endif
            // In VR mode, we want to cover the entire surface
            Stacks::withIdentity([&] {
                oria::renderGeometry(plane, planeProgram, LambdaList({ [&] {
                    Uniform<vec2>(*planeProgram, "UvMultiplier").Set(vec2(texRes));
                } }));
            });
#ifdef USE_RIFT
        } else {
            // In 2D mode, we want to render it as a window behind the UI
            Context::Clear().ColorBuffer();
            MatrixStack & mv = Stacks::modelview();
            static vec3 scale = vec3(3.0f, 3.0f / (textureSize().x / textureSize().y), 3.0f);
            static vec3 trans = vec3(0, 0, -3.5);
            static mat4 rot = glm::rotate(mat4(), PI / 2.0f, Vectors::Y_AXIS);


            for (int i = 0; i < 4; ++i) {
                mv.withPush([&] {
                    for (int j = 0; j < i; ++j) {
                        mv.postMultiply(rot);
                    }
                    mv.translate(trans);
                    mv.scale(scale);
                    oria::renderGeometry(plane, planeProgram, LambdaList({ [&] {
                        Uniform<vec2>(*planeProgram, "UvMultiplier").Set(vec2(texRes));
                    } }));
                    oria::renderGeometry(plane, planeProgram);
                });
                }
            }
#endif

        if (animationValue > 0.0f) {
            MatrixStack & mv = Stacks::modelview();
            Texture::Active(0);
            //      oria::viewport(textureSize());
            mv.withPush([&] {
                mv.translate(vec3(0, 0, -1));
                mv.scale(vec3(1.0f, animationValue, 1.0f));
                uiFramebuffer->BindColor();
                oria::renderGeometry(uiShape, uiProgram);
            });
        }
            }

    public slots:

signals :
    void fpsUpdated(float);
        };

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
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

class App : public QApplication {
    Q_OBJECT

        QWidget desktopWindow;

public:
    App(int argc, char ** argv) : QApplication(argc, argv) {
        Q_INIT_RESOURCE(Common);
        Q_INIT_RESOURCE(VideoVR);
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
        ORIGINAL_MESSAGE_HANDLER = qInstallMessageHandler(myMessageOutput);
    }

    virtual ~App() {
        qInstallMessageHandler(ORIGINAL_MESSAGE_HANDLER);
        LOG_FILE->close();
    }

private:
    void setupDesktopWindow() {
        desktopWindow.setLayout(new QFormLayout());
        QLabel * label = new QLabel("Your Oculus Rift is now active.  Please put on your headset.  Share and enjoy");
        desktopWindow.layout()->addWidget(label);
        desktopWindow.show();
    }
};

MAIN_DECL { 
    try {
#ifdef USE_RIFT
        ovr_Initialize();
#endif

        QT_APP_WITH_ARGS(App);

        MainWindow * riftRenderWidget;
        riftRenderWidget = new MainWindow();
        riftRenderWidget->start();
        riftRenderWidget->requestActivate();
        int result = app.exec(); 

        riftRenderWidget->stop();
        riftRenderWidget->makeCurrent();
        Platform::runShutdownHooks();
        delete riftRenderWidget;

        ovr_Shutdown();

        return result;
    } catch (std::exception & error) {
        SAY_ERR(error.what());
    } catch (const std::string & error) {
        SAY_ERR(error.c_str());
    }
    return -1;
}

#include "VideoVR.moc"
