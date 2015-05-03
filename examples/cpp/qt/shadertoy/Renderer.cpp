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
#include "Shadertoy.h"
#include "Renderer.h"

using namespace oglplus;

void Renderer::setup(QOpenGLContext * context) {
    this->context = context;
    initTextureCache();

    setShaderSourceInternal(readFileToString(":/shaders/default.fs"));
    assert(shadertoyProgram);
    skybox = oria::loadSkybox(shadertoyProgram);

    Platform::addShutdownHook([&] {
        textureCache.clear();
        shadertoyProgram.reset();
        vertexShader.reset();
        fragmentShader.reset();
        skybox.reset();
    });
}

void Renderer::initTextureCache() {
    using namespace shadertoy;
    QRegExp re("(tex|cube)(\\d+)(_0)?\\.(png|jpg)");

    for (int i = 0; i < TEXTURES.size(); ++i) {
        QString path = TEXTURES.at(i);
        QString fileName = path.split("/").back();
        qDebug() << "Loading texture from " << path;
        TextureData & cacheEntry = textureCache[path];
        cacheEntry.tex = oria::load2dTexture(readFileToVector(":" + path), cacheEntry.size);
        canonicalPathMap["qrc:" + path] = path;

        // Backward compatibility
        if (re.exactMatch(fileName)) {
            int textureId = re.cap(2).toInt();

            QString alias = QString("preset://tex/%1").arg(textureId);
            qDebug() << "Adding alias for " << path << " to " << alias;
            canonicalPathMap[alias] = path;

            alias = QString("preset://tex/%1").arg(textureId, 2, 10, QChar('0'));
            qDebug() << "Adding alias for " << path << " to " << alias;
            canonicalPathMap[alias] = path;
        }
    }

    for (int i = 0; i < CUBEMAPS.size(); ++i) {
        QString pathTemplate = CUBEMAPS.at(i);
        QString path = pathTemplate.arg(0);
        QString fileName = path.split("/").back();
        qDebug() << "Processing path " << path;
        TextureData & cacheEntry = textureCache[path];
        cacheEntry.tex = oria::loadCubemapTexture([&](int i) {
            QString texturePath = pathTemplate.arg(i);
            ImagePtr image = oria::loadImage(readFileToVector(":" + texturePath), false);
            if (image) {
                cacheEntry.size = uvec2(image->Width(), image->Height());
            }
            return image;
        });
        canonicalPathMap["qrc:" + path] = path;

        // Backward compatibility
        if (re.exactMatch(fileName)) {
            int textureId = re.cap(2).toInt();
            QString alias = QString("preset://cube/%1").arg(textureId);
            qDebug() << "Adding alias for " << path << " to " << alias;
            canonicalPathMap[alias] = path;

            alias = QString("preset://cube/%1").arg(textureId, 2, 10, QChar('0'));
            qDebug() << "Adding alias for " << path << " to " << alias;
            canonicalPathMap[alias] = path;
        }
    }

    std::for_each(canonicalPathMap.begin(), canonicalPathMap.end(), [&](CanonicalPathMap::const_reference & entry) {
        qDebug() << entry.second << "\t" << entry.first;
    });
}

void Renderer::render() {
    Context::Clear().ColorBuffer();
    if (!shadertoyProgram) {
        return;
    }
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&] {
        mv.untranslate();
        oria::renderGeometry(skybox, shadertoyProgram, uniformLambdas);
    });
    for (int i = 0; i < 4; ++i) {
        oglplus::DefaultTexture().Active(0);
        DefaultTexture().Bind(Texture::Target::_2D);
        DefaultTexture().Bind(Texture::Target::CubeMap);
    }
    oglplus::Texture::Active(0);
}

void Renderer::updateUniforms() {
    using namespace shadertoy;
    typedef std::map<std::string, GLuint> Map;
    Map activeUniforms = oria::getActiveUniforms(shadertoyProgram);
    shadertoyProgram->Bind();
    //    UNIFORM_DATE;
    for (int i = 0; i < 4; ++i) {
        const char * uniformName = shadertoy::UNIFORM_CHANNELS[i];
        if (activeUniforms.count(uniformName)) {
            context->functions()->glUniform1i(activeUniforms[uniformName], i);
        }
        if (channels[i].texture) {
            if (activeUniforms.count(UNIFORM_CHANNEL_RESOLUTIONS[i])) {
                Uniform<vec3>(*shadertoyProgram, UNIFORM_CHANNEL_RESOLUTIONS[i]).Set(channels[i].resolution);
            }

        }
    }
    NoProgram().Bind();

    uniformLambdas.clear();
    if (activeUniforms.count(UNIFORM_GLOBALTIME)) {
        uniformLambdas.push_back([&] {
            Uniform<GLfloat>(*shadertoyProgram, UNIFORM_GLOBALTIME).Set(Platform::elapsedSeconds() - startTime);
        });
    }

    if (activeUniforms.count(UNIFORM_RESOLUTION)) {
        uniformLambdas.push_back([&] {
            vec3 res = vec3(resolution, 0);
            Uniform<vec3>(*shadertoyProgram, UNIFORM_RESOLUTION).Set(res);
        });
    }

#ifdef USE_RIFT
    if (activeUniforms.count(shadertoy::UNIFORM_POSITION)) {
        uniformLambdas.push_back([&] {
            Uniform<vec3>(*shadertoyProgram, shadertoy::UNIFORM_POSITION).Set(position);
        });
    }
#endif

    for (int i = 0; i < 4; ++i) {
        if (activeUniforms.count(UNIFORM_CHANNELS[i]) && channels[i].texture) {
            uniformLambdas.push_back([=] {
                if (this->channels[i].texture) {
                    Texture::Active(i);
                    this->channels[i].texture->Bind(channels[i].target);
                }
            });
        }
    }
}

bool Renderer::setShaderSourceInternal(QString source) {
    try {
        position = vec3();
        if (!vertexShader) {
            QString vertexShaderSource = readFileToString(":/shaders/default.vs").toLocal8Bit().constData();
            vertexShader = VertexShaderPtr(new VertexShader());
            vertexShader->Source(vertexShaderSource.toLocal8Bit().constData());
            vertexShader->Compile();
        }

        QString header = shadertoy::SHADER_HEADER;
        for (int i = 0; i < 4; ++i) {
            const Channel & channel = channels[i];
            QString line; line.sprintf("uniform sampler%s iChannel%d;\n",
                channel.target == Texture::Target::CubeMap ? "Cube" : "2D", i);
            header += line;
        }
        header += shadertoy::LINE_NUMBER_HEADER;
        FragmentShaderPtr newFragmentShader(new FragmentShader());
        bool oldStyle = source.contains("void main(");
        source.
            replace(QRegExp("\\t"), "  ").
            replace(QRegExp("\\bgl_FragColor\\b"), "FragColor").
            replace(QRegExp("\\btexture2D\\b"), "texture").
            replace(QRegExp("\\btextureCube\\b"), "texture");
        source.insert(0, header);

        if (!oldStyle) {
            source.append(shadertoy::SHADER_FOOTER);
        }
        QByteArray qb = source.toLocal8Bit();
        GLchar * fragmentSource = (GLchar*)qb.data();
        StrCRef src(fragmentSource);
        newFragmentShader->Source(GLSLSource(src));
        newFragmentShader->Compile();
        ProgramPtr result(new Program());
        result->AttachShader(*vertexShader);
        result->AttachShader(*newFragmentShader);

        result->Link();
        shadertoyProgram.swap(result);
        if (!skybox) {
            skybox = oria::loadSkybox(shadertoyProgram);
        }
        fragmentShader.swap(newFragmentShader);
        updateUniforms();
        startTime = Platform::elapsedSeconds();
        emit compileSuccess();
    } catch (ProgramBuildError & err) {
        emit compileError(QString(err.Log().c_str()));
        return false;
    }
    return true;
}

Renderer::TextureData Renderer::loadTexture(QString source) {
    qDebug() << "Looking for texture " << source;
    while (canonicalPathMap.count(source)) {
        source = canonicalPathMap[source];
    }

    if (!textureCache.count(source)) {
        qWarning() << "Texture " << source << " not found, loading";
        std::vector<uint8_t> textureData = readFileToVector(source);
        if (!textureData.empty()) {
            textureCache[source].tex = oria::load2dTexture(textureData, textureCache[source].size);
        } else {
            qWarning() << "Could not load texture";
        }
    }
    return textureCache[source];
}

void Renderer::setChannelTextureInternal(int channel, shadertoy::ChannelInputType type, const QString & textureSource) {
    using namespace oglplus;
    if (textureSource == channelSources[channel]) {
        return;
    }

    channelSources[channel] = textureSource;

    if (QUrl() == textureSource) {
        channels[channel].texture.reset();
        channels[channel].target = Texture::Target::_2D;
        return;
    }

    Channel newChannel;
    uvec2 size;
    auto texData = loadTexture(textureSource);
    newChannel.texture = texData.tex;
    switch (type) {
    case shadertoy::ChannelInputType::TEXTURE:
        newChannel.target = Texture::Target::_2D;
        newChannel.resolution = vec3(texData.size, 0);
        break;

    case shadertoy::ChannelInputType::CUBEMAP:
        newChannel.target = Texture::Target::CubeMap;
        newChannel.resolution = vec3(texData.size.x);
        break;

    case shadertoy::ChannelInputType::VIDEO:
        // FIXME, not supported
        break;

    case shadertoy::ChannelInputType::AUDIO:
        // FIXME, not supported
        break;
    }

    channels[channel] = newChannel;
}

void Renderer::setShaderInternal(const shadertoy::Shader & shader) {
    for (int i = 0; i < shadertoy::MAX_CHANNELS; ++i) {
        setChannelTextureInternal(i, shader.channelTypes[i], shader.channelTextures[i]);
    }
    setShaderSourceInternal(shader.fragmentSource);
}
