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
#include "MainWindow.h"
#include "ShadertoyQt.h"

#include <QQuickTextDocument>
#include <QGuiApplication>
#include <QQmlContext>
#include <QDeclarativeEngine>

using namespace oglplus;

QDir CONFIG_DIR;
static const float ROOT_2 = sqrt(2.0f);
static const float INV_ROOT_2 = 1.0f / ROOT_2;
static uvec2 UI_SIZE(1280, 720);
static float UI_ASPECT = aspect(vec2(UI_SIZE));
static float UI_INVERSE_ASPECT = 1.0f / UI_ASPECT;

struct Preset {
    const Resource res;
    const char * name;
    Preset(Resource res, const char * name) : res(res), name(name) {};
};

const QStringList PRESETS({
    ":/shaders/default.xml",
    ":/shaders/XljGR3.json",
    ":/shaders/4df3DS.json",
    ":/shaders/4dfGzs.json",
    ":/shaders/4djGWR.json",
    ":/shaders/4ds3zn.json",
    ":/shaders/4dXGRM_flying_steel_cubes.xml",
    // ":/shaders/4sBGD1.json",
    // ":/shaders/4slGzn.json",

    ":/shaders/4sX3R2.json", // Monster
    ":/shaders/4sXGRM_oceanic.xml",
    ":/shaders/4tXGDn.json", // Morphing
    //":/shaders/ld23DG_crazy.xml",
    ":/shaders/ld2GRz.json", // meta-balls
    // ":/shaders/ldfGzr.json",
    // ":/shaders/ldj3Dm.json", // fish swimming
    ":/shaders/ldl3zr_mobius_balls.xml",
    // ":/shaders/ldSGRW.json",
    // ":/shaders/lsl3W2.json",
    ":/shaders/lss3WS_relentless.xml",
    // ":/shaders/lts3Wn.json",
    ":/shaders/MdX3Rr.json", // elevated
    // ":/shaders/MsBGRh.json",
    // ":/shaders/MsSGD1_hand_drawn_sketch.xml",
    ":/shaders/MsXGz4.json", // cubemap
    ":/shaders/MsXGzM.json", // voronoi rocks
    // ":/shaders/MtfGR8_snowglobe.xml",
    // ":/shaders/XdBSzd.json",
    ":/shaders/Xlf3D8.json", // sci-fi
    ":/shaders/XsBSRG_morning_city.xml",
    ":/shaders/XsjXR1.json", // worms
    ":/shaders/XslXW2.json", // mechanical (2D)
    // ":/shaders/XsSSRW.json"
});

MainWindow::MainWindow() {
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

    fetcher.fetchNetworkShaders();

    connect(&timer, &QTimer::timeout, this, &MainWindow::onTimer);
    timer.start(100);
    setupOffscreenUi();
    onLoadPreset(0);
    Platform::addShutdownHook([&] {
        shaderFramebuffer.reset();
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

void MainWindow::stop() {
    QRiftWindow::stop();
    delete uiWindow;
    uiWindow = nullptr;
    makeCurrent();
    Platform::runShutdownHooks();
}

void MainWindow::setup() {
    QRiftWindow::setup();

    renderer.setup(context());
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
    uiFramebuffer->Init(UI_SIZE);

    shaderFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
    shaderFramebuffer->Init(textureSize());

    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
}

void MainWindow::setupOffscreenUi() {
#ifdef USE_RIFT
    this->endFrameLock = &uiWindow->renderLock;
#endif
    qApp->setFont(QFont("Arial", 14, QFont::Bold));
    uiWindow->pause();
    uiWindow->setup(QSize(UI_SIZE.x, UI_SIZE.y), context());
    {
        QStringList dataList;
        foreach(const QString path, PRESETS) {
            shadertoy::Shader shader = shadertoy::loadShaderFile(path);
            dataList.append(shader.name);
        }
        auto qmlContext = uiWindow->m_qmlEngine->rootContext();
        qmlContext->setContextProperty("presetsModel", QVariant::fromValue(dataList));
        QUrl url = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/shaders");
        qmlContext->setContextProperty("userPresetsFolder", url);
    }
    uiWindow->setProxyWindow(this);

    QUrl qml = QUrl("qrc:/layouts/Combined.qml");
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
        highlighter.setDocument(
            editorControl->property("textDocument").value<QQuickTextDocument*>()->textDocument());
    }

    QObject::connect(uiWindow->m_rootItem, SIGNAL(toggleUi()),
        this, SLOT(onToggleUi()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(channelTextureChanged(int, int, QString)),
        this, SLOT(onChannelTextureChanged(int, int, QString)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(shaderSourceChanged(QString)),
        this, SLOT(onShaderSourceChanged(QString)));

    // FIXME add confirmation for when the user might lose edits.
    QObject::connect(uiWindow->m_rootItem, SIGNAL(loadPreset(int)),
        this, SLOT(onLoadPreset(int)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(loadNextPreset()),
        this, SLOT(onLoadNextPreset()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(loadPreviousPreset()),
        this, SLOT(onLoadPreviousPreset()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(loadShaderFile(QString)),
        this, SLOT(onLoadShaderFile(QString)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(saveShaderXml(QString)),
        this, SLOT(onSaveShaderXml(QString)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(recenterPose()),
        this, SLOT(onRecenterPosition()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(modifyTextureResolution(double)),
        this, SLOT(onModifyTextureResolution(double)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(modifyPositionScale(double)),
        this, SLOT(onModifyPositionScale(double)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(resetPositionScale()),
        this, SLOT(onResetPositionScale()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(toggleEyePerFrame()),
        this, SLOT(onToggleEyePerFrame()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(epfModeChanged(bool)),
        this, SLOT(onEpfModeChanged(bool)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(startShutdown()),
        this, SLOT(onShutdown()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(restartShader()),
        this, SLOT(onRestartShader()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(newShaderFilepath(QString)),
        this, SLOT(onNewShaderFilepath(QString)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(newShaderHighlighted(QString)),
        this, SLOT(onNewShaderHighlighted(QString)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(newPresetHighlighted(int)),
        this, SLOT(onNewPresetHighlighted(int)));

    QObject::connect(&renderer, &Renderer::compileSuccess, this, [&] {
        setItemProperty("errorFrame", "height", 0);
        setItemProperty("errorFrame", "visible", false);
        setItemProperty("compileErrors", "text", "");
        setItemProperty("shaderTextFrame", "errorMargin", 0);
    });

    QObject::connect(&renderer, &Renderer::compileError, this, [&](const QString& errors) {
        setItemProperty("errorFrame", "height", 128);
        setItemProperty("errorFrame", "visible", true);
        setItemProperty("compileErrors", "text", errors);
        setItemProperty("shaderTextFrame", "errorMargin", 8);
    });

    QObject::connect(&animation, &QVariantAnimation::valueChanged, this, [&](const QVariant & val) {
        animationValue = val.toFloat();
    });

    connect(this, &MainWindow::fpsUpdated, this, [&](float fps) {
        setItemText("fps", QString().sprintf("%0.0f", fps));
    });

    setItemText("res", QString().sprintf("%0.2f", texRes));
    uiWindow->setSourceSize(size());
}

QVariant MainWindow::getItemProperty(const QString & itemName, const QString & property) {
    QQuickItem * item = uiWindow->m_rootItem->findChild<QQuickItem*>(itemName);
    if (nullptr != item) {
        return item->property(property.toLocal8Bit());
    } else {
        qWarning() << "Could not find item " << itemName << " on which to set property " << property;
    }
    return QVariant();
}

void MainWindow::setItemProperty(const QString & itemName, const QString & property, const QVariant & value) {
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

void MainWindow::setItemText(const QString & itemName, const QString & text) {
    setItemProperty(itemName, "text", text);
}

QString MainWindow::getItemText(const QString & itemName) {
    return getItemProperty(itemName, "text").toString();
}

void MainWindow::onToggleUi() {
    uiVisible = !uiVisible;
    setItemProperty("shaderTextEdit", "readOnly", !uiVisible);
    if (uiVisible) {
        animation.stop();
        animation.setStartValue(0.0);
        animation.setEndValue(1.0);
        animation.setEasingCurve(QEasingCurve::OutBack);
        animation.setDuration(500);
        animation.start();
        savedEyePosScale = eyeOffsetScale;
        eyeOffsetScale = 0.0f;
        uiWindow->resume();
    } else {
        animation.stop();
        animation.setStartValue(1.0);
        animation.setEndValue(0.0);
        animation.setEasingCurve(QEasingCurve::InBack);
        animation.setDuration(500);
        animation.start();
        eyeOffsetScale = savedEyePosScale;
        uiWindow->pause();
    }
}

void MainWindow::onLoadNextPreset() {
    static const int PRESETS_SIZE = PRESETS.size();
    int newPreset = (activePresetIndex + 1) % PRESETS_SIZE;
    onLoadPreset(newPreset);
}

void MainWindow::onFontSizeChanged(int newSize) {
    settings.setValue("fontSize", newSize);
}

void MainWindow::onLoadPreviousPreset() {
    static const int PRESETS_SIZE = PRESETS.size();
    int newPreset = (activePresetIndex + PRESETS_SIZE - 1) % PRESETS_SIZE;
    onLoadPreset(newPreset);
}

void MainWindow::onLoadPreset(int index) {
    activePresetIndex = index;
    loadShader(shadertoy::loadShaderFile(PRESETS.at(index)));
}

void MainWindow::onLoadShaderFile(const QString & shaderPath) {
    qDebug() << "Loading shader from " << shaderPath;
    loadShader(shadertoy::loadShaderFile(shaderPath));
}

void MainWindow::onNewShaderFilepath(const QString & shaderPath) {
    QDir newDir(shaderPath);
    QUrl url = QUrl::fromLocalFile(newDir.absolutePath());
    //    setItemProperty("userPresetsModel", "folder", url);
    auto qmlContext = uiWindow->m_qmlEngine->rootContext();
    qmlContext->setContextProperty("userPresetsFolder", url);
}

void MainWindow::onNewShaderHighlighted(const QString & shaderPath) {
    qDebug() << "New shader highlighted " << shaderPath;
    QString previewPath = shaderPath;
    previewPath.replace(QRegularExpression("\\.(json|xml)$"), ".jpg");
    setItemProperty("previewImage", "source", QFile::exists(previewPath) ? QUrl::fromLocalFile(previewPath) : QUrl());
    if (shaderPath.endsWith(".json")) {
        setItemProperty("loadRoot", "activeShaderString", readFileToString(shaderPath));
    } else {
        setItemProperty("loadRoot", "activeShaderString", "");
    }
}

void MainWindow::onNewPresetHighlighted(int presetId) {
    if (-1 != presetId && presetId < PRESETS.size()) {
        QString path = PRESETS.at(presetId);
        QString previewPath = path;
        previewPath.replace(QRegularExpression("\\.(json|xml)$"), ".jpg");
        setItemProperty("previewImage", "source", "qrc" + previewPath);
        qDebug() << previewPath;
    }
    //previewPath.replace(QRegularExpression("\\.(json|xml)$"), ".jpg");
    //setItemProperty("previewImage", "source", QFile::exists(previewPath) ? QUrl::fromLocalFile(previewPath) : QUrl());
    //if (shaderPath.endsWith(".json")) {
    //  setItemProperty("loadRoot", "activeShaderString", readFileToString(shaderPath));
    //} else {
    //  setItemProperty("loadRoot", "activeShaderString", "");
    //}
}

void MainWindow::onSaveShaderXml(const QString & shaderPath) {
    Q_ASSERT(!shaderPath.isEmpty());
    //shadertoy::saveShaderXml(activeShader);
    activeShader.name = shaderPath.toLocal8Bit();
    activeShader.fragmentSource = getItemText("shaderTextEdit").toLocal8Bit();
    QString destinationFile = configPath.absoluteFilePath(QString("shaders/")
        + shaderPath
        + ".xml");
    qDebug() << "Saving shader to " << destinationFile;
    shadertoy::saveShaderXml(destinationFile, activeShader);
}

void MainWindow::onChannelTextureChanged(const int & channelIndex, const int & channelType, const QString & texturePath) {
    qDebug() << "Requesting texture from path " << texturePath;
    queueRenderThreadTask([&, channelIndex, channelType, texturePath] {
        qDebug() << texturePath;
        activeShader.channelTypes[channelIndex] = (shadertoy::ChannelInputType)channelType;
        activeShader.channelTextures[channelIndex] = texturePath.toLocal8Bit();
        renderer.setChannelTextureInternal(channelIndex,
            (shadertoy::ChannelInputType)channelType,
            texturePath);
        renderer.updateUniforms();
    });
}

void MainWindow::onShaderSourceChanged(const QString & shaderSource) {
    queueRenderThreadTask([&, shaderSource] {
        renderer.setShaderSourceInternal(shaderSource);
        renderer.updateUniforms();
        activeShader.vrEnabled = shaderSource.contains("#pragma vr");
    });
}

void MainWindow::onRecenterPosition() {
#ifdef USE_RIFT
    queueRenderThreadTask([&] {
        ovrHmd_RecenterPose(hmd);
    });
#endif
}

void MainWindow::onModifyTextureResolution(double scale) {
    float newRes = scale * texRes;
    newRes = std::max(0.1f, std::min(1.0f, newRes));
    if (newRes != texRes) {
        queueRenderThreadTask([&, newRes] {
            texRes = newRes;
        });
        setItemText("res", QString().sprintf("%0.2f", newRes));
    }
}

void MainWindow::onModifyPositionScale(double scale) {
    float newPosScale = scale * eyeOffsetScale;
    queueRenderThreadTask([&, newPosScale] {
        eyeOffsetScale = newPosScale;
    });
    // FIXME update the UI
    setItemText("eps", QString().sprintf("%0.2f", newPosScale));
}

void MainWindow::onResetPositionScale() {
    queueRenderThreadTask([&] {
        eyeOffsetScale = 1.0f;
    });
    // FIXME update the UI
    setItemText("eps", QString().sprintf("%0.2f", 1.0f));
}

void MainWindow::onToggleEyePerFrame() {
#ifdef USE_RIFT
    onEpfModeChanged(!eyePerFrameMode);
#endif
}

void MainWindow::onEpfModeChanged(bool checked) {
    bool newEyePerFrameMode = checked;
#ifdef USE_RIFT
    queueRenderThreadTask([&, newEyePerFrameMode] {
        eyePerFrameMode = newEyePerFrameMode;
    });
#endif
    setItemProperty("epf", "checked", newEyePerFrameMode);
}

void MainWindow::onRestartShader() {
    queueRenderThreadTask([&] {
        renderer.restart();
    });
}

void MainWindow::onShutdown() {
    QApplication::instance()->quit();
}

void MainWindow::onTimer() {
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

void MainWindow::loadShader(const shadertoy::Shader & shader) {
    assert(!shader.fragmentSource.isEmpty());
    activeShader = shader;
    setItemText("shaderTextEdit", QString(shader.fragmentSource).replace(QString("\t"), QString("  ")));
    setItemText("shaderName", shader.name);
    for (int i = 0; i < 4; ++i) {
        QString texturePath = renderer.canonicalTexturePath(activeShader.channelTextures[i]);
        qDebug() << "Setting channel " << i << " to texture " << texturePath;
        setItemProperty(QString().sprintf("channel%d", i), "source", "qrc:" + texturePath);
    }

    // FIXME update the channel texture buttons
    queueRenderThreadTask([&, shader] {
        renderer.setShaderInternal(shader);
        renderer.updateUniforms();
    });
}

void MainWindow::loadFile(const QString & file) {
    loadShader(shadertoy::loadShaderFile(file));
}

void MainWindow::updateFps(float fps) {
    emit fpsUpdated(fps);
}

///////////////////////////////////////////////////////
//
// Event handling customization
// 
void MainWindow::mouseMoveEvent(QMouseEvent * me) {
    // Make sure we don't show the system cursor over the window
    qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
    QRiftWindow::mouseMoveEvent(me);
}

bool MainWindow::event(QEvent * e) {
    if (uiWindow) {
        if (uiWindow->interceptEvent(e)) {
            return true;
        }
    }
    return QRiftWindow::event(e);
}

void MainWindow::resizeEvent(QResizeEvent *e) {
    QRiftWindow::resizeEvent(e);
    uiWindow->setSourceSize(e->size());
}

///////////////////////////////////////////////////////
//
// Rendering functionality
//
void MainWindow::perFrameRender() {
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
            uiFramebuffer->Pushed([&] {
                Context::Clear().ColorBuffer();
                oria::viewport(UI_SIZE);
                // Clear out the projection and modelview here.
                Stacks::withIdentity([&] {
                    glBindTexture(GL_TEXTURE_2D, currentUiTexture);
                    oria::renderGeometry(plane, uiProgram);

                    // Render the mouse sprite on the UI
                    QSizeF mp = uiWindow->getMousePosition().load();
                    mv.translate(vec3(mp.width(), mp.height(), 0.0f));
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

void MainWindow::perEyeRender() {
    // Render the shadertoy effect into a framebuffer, possibly at a
    // smaller resolution than recommended
    shaderFramebuffer->Pushed([&] {
        Context::Clear().ColorBuffer();
        oria::viewport(renderSize());
        renderer.setResolution(renderSize());
#ifdef USE_RIFT
        renderer.setPosition(ovr::toGlm(getEyePose().Position) * eyeOffsetScale);
#endif
        renderer.render();
    });
    oria::viewport(textureSize());

    // Now re-render the shader output to the screen.
    shaderFramebuffer->BindColor(Texture::Target::_2D);
#ifdef USE_RIFT
    if (activeShader.vrEnabled) {
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
            mv.translate(vec3(0, 0, -1.2));
            mv.scale(vec3(1.0f, animationValue, 1.0f));
            uiFramebuffer->BindColor();
            oria::renderGeometry(uiShape, uiProgram);
        });
    }
}

void MainWindow::onSixDofMotion(const vec3 & tr, const vec3 & mo) {
    SAY("%f, %f, %f", tr.x, tr.y, tr.z);
    queueRenderThreadTask([&, tr, mo] {
        // FIXME 
        // renderer.setPosition( position += tr;
    });
}
