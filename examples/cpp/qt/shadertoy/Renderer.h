#pragma once

class ShadertoyRenderer : public QRiftWindow {
  Q_OBJECT
protected:
  struct Channel {
    oglplus::Texture::Target target;
    TexturePtr texture;
    vec3 resolution;
  };
  struct TextureData {
    TexturePtr tex;
    uvec2 size;
  };

  typedef std::map<QString, TextureData> TextureMap;
  typedef std::map<QString, QString> CanonicalPathMap;
  CanonicalPathMap canonicalPathMap;
  TextureMap textureCache;

  // The currently active input channels
  Channel channels[4];
  QString channelSources[4];
  bool vrMode{ false };

  // The shadertoy rendering resolution scale.  1.0 means full resolution
  // as defined by the Oculus SDK as the ideal offscreen resolution
  // pre-distortion
  float texRes{ 1.0f };

  float eyePosScale{ 1.0f };
  float startTime{ 0.0f };
  // The current fragment source
  LambdaList uniformLambdas;
  // Contains the current 'camera position'
  vec3 position;
  // Geometry for the skybox used to render the scene
  ShapeWrapperPtr skybox;
  // A vertex shader shader, constant throughout the application lifetime
  VertexShaderPtr vertexShader;
  // The fragment shader used to render the shadertoy effect, as loaded
  // from a preset or created or edited by the user
  FragmentShaderPtr fragmentShader;
  // The compiled shadertoy program
  ProgramPtr shadertoyProgram;

  virtual void setup();
  void initTextureCache();
  void renderShadertoy();
  void updateUniforms();
  vec2 textureSize();
  uvec2 renderSize();

  virtual bool setShaderSourceInternal(QString source);
  virtual TextureData loadTexture(QString source);
  virtual void setChannelTextureInternal(int channel, shadertoy::ChannelInputType type, const QString & textureSource);
  virtual void setShaderInternal(const shadertoy::Shader & shader);

signals:
  void compileError(const QString & source);
  void compileSuccess();
};
