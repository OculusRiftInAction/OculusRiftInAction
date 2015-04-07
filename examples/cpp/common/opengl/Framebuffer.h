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


// A wrapper for constructing and using a
struct FramebufferWrapper {
  glm::uvec2              size;
  oglplus::Framebuffer    fbo;
  oglplus::Texture        color;
  oglplus::Renderbuffer   depth;

  FramebufferWrapper() {
  }

  FramebufferWrapper(const glm::uvec2 & size) {
    init(size);
  }

  void initColor() {
      using namespace oglplus;
      Context::Bound(Texture::Target::_2D, color)
          .MinFilter(TextureMinFilter::Linear)
          .MagFilter(TextureMagFilter::Linear)
          .WrapS(TextureWrap::ClampToEdge)
          .WrapT(TextureWrap::ClampToEdge)
          .Image2D(
          0, PixelDataInternalFormat::RGBA8,
          size.x, size.y,
          0, PixelDataFormat::RGB, PixelDataType::UnsignedByte, nullptr
          );
  }

  void initDepth() {
      using namespace oglplus;
      Context::Bound(Renderbuffer::Target::Renderbuffer, depth)
          .Storage(
          PixelDataInternalFormat::DepthComponent,
          size.x, size.y);
  }

  void initDone() {
      using namespace oglplus;
      Bound([&] {
          fbo.AttachTexture(Framebuffer::Target::Draw, FramebufferAttachment::Color, color, 0);
          fbo.AttachRenderbuffer(Framebuffer::Target::Draw, FramebufferAttachment::Depth, depth);
          fbo.Complete(Framebuffer::Target::Draw);
      });
  }
  
  void init(const glm::uvec2 & size) {
    using namespace oglplus;
    this->size = size;
    initColor();
    initDepth();
    initDone();
  }

  void Bind(oglplus::Framebuffer::Target target = oglplus::Framebuffer::Target::Draw) {
    fbo.Bind(target);
    Viewport(); 
  }

  static void Unbind(oglplus::Framebuffer::Target target = oglplus::Framebuffer::Target::Draw) {
    oglplus::DefaultFramebuffer().Bind(target);
  }

  void Viewport() {
    oglplus::Context::Viewport(0, 0, size.x, size.y);
  }

  template <typename F> 
  void Bound(F f, oglplus::Framebuffer::Target target = oglplus::Framebuffer::Target::Draw) {
    oglplus::FramebufferName oldFbo = oglplus::Framebuffer::Binding(target);
    Bind(target);
    f();
    oglplus::Framebuffer::Bind(target, oldFbo);
  }

  void BindColor(oglplus::Texture::Target target = oglplus::Texture::Target::_2D) {
    color.Bind(target);
  }
};

typedef std::shared_ptr<FramebufferWrapper> FramebufferWrapperPtr;
