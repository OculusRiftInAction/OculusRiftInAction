/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.

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

#ifndef GL_ZERO
#error "You must include the GL headers before including this file"
#endif

#include "GlTexture.h"
#include "GlDebug.h"

namespace GL {

struct FrameBuffer {
    GLuint frameBuffer;
    TexturePtr texture;
    //GLuint texture;
    GLuint depthBuffer;
    GLsizei width, height;

    FrameBuffer() : frameBuffer(0), texture(nullptr), depthBuffer(0), width(0), height(0) {
    }

    FrameBuffer(TexturePtr texture) : frameBuffer(0), texture(texture), depthBuffer(0), width(0), height(0) {
    }

    virtual ~FrameBuffer() {
        if (frameBuffer) {
            glDeleteFramebuffers(1, &frameBuffer);
            GL_CHECK_ERROR;
        }

        if (depthBuffer) {
            glDeleteRenderbuffers(1, &depthBuffer);
            GL_CHECK_ERROR;
        }
    }

    void init(GLsizei width, GLsizei height) {
        this->width = width;
        this->height = height;

        glGenFramebuffers(1, &frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        GL_CHECK_ERROR;

        if (!texture) {
            texture = TexturePtr(new Texture());
            texture->init();
            texture->bind();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            ///glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            texture->unbind();
            GL_CHECK_ERROR;
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0);
        GL_CHECK_ERROR;

        glGenRenderbuffers(1, &depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
        GL_CHECK_ERROR;

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
        GL_CHECK_ERROR;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void activate() {
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        glViewport(0, 0, width, height);
    }
    void deactivate() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        texture->bind();
        glGenerateMipmap(GL_TEXTURE_2D);
        texture->unbind();
    }
    TexturePtr & getTexture() {
        return texture;
    }
    TexturePtr detachTexture() {
        auto result = texture;
        texture = nullptr;
        return result;
    }

};

} // GL

