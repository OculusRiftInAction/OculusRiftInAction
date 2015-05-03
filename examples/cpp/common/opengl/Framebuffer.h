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

struct FramebufferWrapper;

class FramebufferStack {
private:
  FramebufferStack();
public:
  static FramebufferStack & instance();
  
  void Push(oglplus::Framebuffer::Target target, FramebufferWrapper * fboWrapper);
  void Pop();
};

// A wrapper for constructing and using a
struct FramebufferWrapper {
  glm::uvec2              size;
  oglplus::Framebuffer    fbo;
  oglplus::Texture        color;
  oglplus::Renderbuffer   depth;

  FramebufferWrapper() {
  }

  FramebufferWrapper(const glm::uvec2 & size) {
    Init(size);
  }
  
  virtual ~FramebufferWrapper() {

  }
  
  virtual void Init(const glm::uvec2 & size) {
    this->size = size;
    initColor();
    initDepth();
    initDone();
  }

  virtual void Push(oglplus::Framebuffer::Target target = oglplus::Framebuffer::Target::Draw) {
    FramebufferStack::instance().Push(target, this);
  }

  static void Pop() {
    FramebufferStack::instance().Pop();
  }
  
  void Viewport() {
    oglplus::Context::Viewport(0, 0, size.x, size.y);
  }
  
  template <typename F>
  void Pushed(F f) {
    Pushed(oglplus::Framebuffer::Target::Draw, f);
  }

  template <typename F>
  void Pushed(oglplus::Framebuffer::Target target, F f) {
    Push(target);
    f();
    Pop();
  }

  void BindColor(oglplus::Texture::Target target = oglplus::Texture::Target::_2D) {
    color.Bind(target);
  }
  
private:
  virtual void onBind(oglplus::Framebuffer::Target target) {}
  virtual void onUnbind(oglplus::Framebuffer::Target target) {}
  
  virtual void initColor() {
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
  
  virtual void initDepth() {
    using namespace oglplus;
    Context::Bound(Renderbuffer::Target::Renderbuffer, depth)
    .Storage(
             PixelDataInternalFormat::DepthComponent,
             size.x, size.y);
  }
  
  virtual void initDone() {
    using namespace oglplus;
    static const Framebuffer::Target target = Framebuffer::Target::Draw;
    Pushed(target, [&] {
      fbo.AttachTexture(target, FramebufferAttachment::Color, color, 0);
      fbo.AttachRenderbuffer(target, FramebufferAttachment::Depth, depth);
      fbo.Complete(target);
    });
  }
  friend class FboState;
};

typedef std::shared_ptr<FramebufferWrapper> FramebufferWrapperPtr;
