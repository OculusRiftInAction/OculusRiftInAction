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

#include "Common.h"

namespace ovr {

  /**
   * Build an OpenGL window, respecting the Rift's current display mode choice of
   * extended or direct HMD.
   */
  GLFWwindow * createRiftRenderingWindow(ovrHmd hmd, glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    outSize = glm::uvec2(hmd->Resolution.w, hmd->Resolution.h);
    outSize /= 2;
    GLFWwindow * window = glfw::createSecondaryScreenWindow(outSize);
    glfwGetWindowPos(window, &outPosition.x, &outPosition.y);
    return window;
  }


  SwapTextureFramebufferWrapper::SwapTextureFramebufferWrapper(const ovrHmd & hmd)
    : RiftFramebufferWrapper(hmd) { }

  SwapTextureFramebufferWrapper::SwapTextureFramebufferWrapper(const ovrHmd & hmd, const glm::uvec2 & size)
    : RiftFramebufferWrapper(hmd) {
      Init(size);
    }

  SwapTextureFramebufferWrapper::~SwapTextureFramebufferWrapper() {
    ovrHmd_DestroySwapTextureSet(hmd, textureSet);
  }

  void SwapTextureFramebufferWrapper::initColor() {
    // FIXME deallocate any previously created swap texutre set if the size has changed.
    if (!OVR_SUCCESS(ovrHmd_CreateSwapTextureSetGL(hmd, GL_RGBA, size.x, size.y, &textureSet))) {
      FAIL("Unable to create swap textures");
    }

    for (int i = 0; i < textureSet->TextureCount; ++i) {
      ovrGLTexture& ovrTex = (ovrGLTexture&)textureSet->Textures[i];
      glBindTexture(GL_TEXTURE_2D, ovrTex.OGL.TexId);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void SwapTextureFramebufferWrapper::initDone() {
    using namespace oglplus;
    Framebuffer::Target target = Framebuffer::Target::Draw;
    Pushed(target, [&]{
      fbo.AttachRenderbuffer(target, FramebufferAttachment::Depth, depth);
      // do nothing with color, because our color attachment will change every frame
    });
  }

  void SwapTextureFramebufferWrapper::onBind(oglplus::Framebuffer::Target target) {
    using namespace oglplus;
    ovrGLTexture& tex = (ovrGLTexture&)(textureSet->Textures[textureSet->CurrentIndex]);
    GLenum glTarget = target == Framebuffer::Target::Draw ? GL_DRAW_FRAMEBUFFER : GL_READ_FRAMEBUFFER;
    glFramebufferTexture2D(glTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.OGL.TexId, 0);
  }
  
  void SwapTextureFramebufferWrapper::onUnbind(oglplus::Framebuffer::Target target) {
    using namespace oglplus;
    GLenum glTarget = target == Framebuffer::Target::Draw ? GL_DRAW_FRAMEBUFFER : GL_READ_FRAMEBUFFER;
    glFramebufferTexture2D(glTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
  }

  void SwapTextureFramebufferWrapper::Increment() {
    ++textureSet->CurrentIndex;
    textureSet->CurrentIndex %= textureSet->TextureCount;
  }
  
  MirrorFramebufferWrapper::MirrorFramebufferWrapper(const ovrHmd & hmd)
    : RiftFramebufferWrapper(hmd) { }

  MirrorFramebufferWrapper::MirrorFramebufferWrapper(const ovrHmd & hmd, const glm::uvec2 & size)
    : RiftFramebufferWrapper(hmd) {
      Init(size);
  }

  MirrorFramebufferWrapper::~MirrorFramebufferWrapper() {
    if (texture) {
      ovrHmd_DestroyMirrorTexture(hmd, (ovrTexture*)texture);
      texture = nullptr;
    }
  }
    
  void MirrorFramebufferWrapper::initColor() {
      if (texture) {
        ovrHmd_DestroyMirrorTexture(hmd, (ovrTexture*)texture);
        texture = nullptr;
      }
      ovrResult result = ovrHmd_CreateMirrorTextureGL(hmd, GL_RGBA, size.x, size.y, (ovrTexture**)&texture);
      Q_ASSERT(OVR_SUCCESS(result));
  }
  
  void MirrorFramebufferWrapper::initDone() {
    using namespace oglplus;
    Framebuffer::Target target = Framebuffer::Target::Read;
    Pushed(target, [&]{
      fbo.AttachRenderbuffer(target, FramebufferAttachment::Depth, depth);
      glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->OGL.TexId, 0);
    });

  }

}

