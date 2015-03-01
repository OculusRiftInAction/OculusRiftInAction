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

#include "QRiftWindow.h"
#include "Shadertoy.h"
#include "Renderer.h"
#include "Fetcher.h"

class MainWindow : public QRiftWindow {
  Q_OBJECT

  typedef std::atomic<GLuint> AtomicGlTexture;
  typedef std::pair<GLuint, GLsync> SyncPair;
  typedef std::queue<SyncPair> TextureTrashcan;
  typedef std::vector<GLuint> TextureDeleteQueue;
  typedef std::mutex Mutex;
  typedef std::unique_lock<Mutex> Lock;

  // A cache of all the input textures available
  QDir configPath;
  QSettings settings;

  //////////////////////////////////////////////////////////////////////////////
  //
  // Offscreen UI
  //
  QOffscreenUi * uiWindow{ new QOffscreenUi() };
  GlslHighlighter highlighter;

  int activePresetIndex{ 0 };
  float savedEyePosScale{ 1.0f };

  //////////////////////////////////////////////////////////////////////////////
  //
  // Shader Rendering information
  //

  shadertoy::Shader activeShader;

  Renderer renderer;

  // We actually render the shader to one FBO for dynamic framebuffer scaling,
  // while leaving the actual texture we pass to the Oculus SDK fixed.
  // This allows us to have a clear UI regardless of the shader performance
  FramebufferWrapperPtr shaderFramebuffer;

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

  Fetcher fetcher;

public:
  MainWindow();
  virtual void stop();

private:
  virtual void setup();
  void setupOffscreenUi();

  QVariant getItemProperty(const QString & itemName, const QString & property);
  void setItemProperty(const QString & itemName, const QString & property, const QVariant & value);

  QString getItemText(const QString & itemName);
  void setItemText(const QString & itemName, const QString & text);

private:
  void loadShader(const shadertoy::Shader & shader);
  void loadFile(const QString & file);
  void updateFps(float fps);

  ///////////////////////////////////////////////////////
  //
  // Event handling customization
  //
  void mouseMoveEvent(QMouseEvent * me);
  bool event(QEvent * e);
  void resizeEvent(QResizeEvent *e);
  QPointF mapWindowToUi(const QPointF & p);

  ///////////////////////////////////////////////////////
  //
  // Rendering functionality
  //
  void perFrameRender();
  void perEyeRender();


private slots:
  void onToggleUi();
  void onLoadNextPreset();
  void onFontSizeChanged(int newSize);
  void onLoadPreviousPreset();
  void onLoadPreset(int index);
  void onLoadShaderFile(const QString & shaderPath);
  void onNewShaderFilepath(const QString & shaderPath);
  void onNewShaderHighlighted(const QString & shaderPath);
  void onNewPresetHighlighted(int presetId);
  void onSaveShaderXml(const QString & shaderPath);
  void onChannelTextureChanged(const int & channelIndex, const int & channelType, const QString & texturePath);
  void onShaderSourceChanged(const QString & shaderSource);
  void onRecenterPosition();
  void onModifyTextureResolution(double scale);
  void onModifyPositionScale(double scale);
  void onResetPositionScale();
  void onToggleEyePerFrame();
  void onEpfModeChanged(bool checked);
  void onRestartShader();
  void onShutdown();
  void onTimer();
  void onSixDofMotion(const vec3 & tr, const vec3 & mo);

signals:
  void fpsUpdated(float);
};
