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

class Renderer : public QObject {
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

    QOpenGLContext * context;

    // The currently active input channels
    Channel channels[4];
    QString channelSources[4];

    //// The shadertoy rendering resolution scale.  1.0 means full resolution
    //// as defined by the Oculus SDK as the ideal offscreen resolution
    //// pre-distortion
    //float texRes{ 1.0f };
    // float eyePosScale{ 1.0f };

    // The current rendering resolution
    vec2 resolution;
    // Contains the current 'camera position'
    vec3 position;
    // The amount of time since we started running
    float startTime{ 0.0f };

    // The current fragment source
    LambdaList uniformLambdas;
    // Geometry for the skybox used to render the scene
    ShapeWrapperPtr skybox;
    // A vertex shader shader, constant throughout the application lifetime
    VertexShaderPtr vertexShader;
    // The fragment shader used to render the shadertoy effect, as loaded
    // from a preset or created or edited by the user
    FragmentShaderPtr fragmentShader;
    // The compiled shadertoy program
    ProgramPtr shadertoyProgram;

    void initTextureCache();

public:
    void setup(QOpenGLContext * context);
    void render();
    void updateUniforms();

    void restart() {
        startTime = Platform::elapsedSeconds();
    }

    void setPosition(const vec3 & position) {
        this->position = position;
    }

    void setResolution(const vec2 & resolution) {
        this->resolution = resolution;
    }

    QString canonicalTexturePath(QString texturePath) {
        while (canonicalPathMap.count(texturePath)) {
            texturePath = canonicalPathMap[texturePath];
        }
        return texturePath;
    }

    virtual bool setShaderSourceInternal(QString source);
    virtual TextureData loadTexture(QString source);
    virtual void setChannelTextureInternal(int channel, shadertoy::ChannelInputType type, const QString & textureSource);
    virtual void setShaderInternal(const shadertoy::Shader & shader);

signals:
    void compileError(const QString & source);
    void compileSuccess();
};
