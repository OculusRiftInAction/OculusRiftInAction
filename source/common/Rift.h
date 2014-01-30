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
#include <array>

class Rift {
public:
  static void getDk1HmdValues(OVR::HMDInfo & hmdInfo);
  static void getRiftPositionAndSize(const OVR::HMDInfo & hmdInfo,
      glm::ivec2 & windowPosition, glm::ivec2 & windowSize);

  static glm::quat getStrabismusCorrection();
  static void setStrabismusCorrection(const glm::quat & q);

  static void getHmdInfo(
    const OVR::Ptr<OVR::DeviceManager> & ovrManager,
    OVR::HMDInfo & out);

  // Fetch a glm style quaternion from an OVR sensor fusion object
  static glm::quat getQuaternion(OVR::SensorFusion & sensorFusion);

  // Fetch a glm vector containing Euler angles from an OVR sensor fusion object
  static glm::vec3 getEulerAngles(OVR::SensorFusion & sensorFusion);
  static glm::vec3 getEulerAngles(const OVR::Quatf & in);

  static glm::vec3 fromOvr(const OVR::Vector3f & in);
  static glm::quat fromOvr(const OVR::Quatf & in);

  static const float MONO_FOV;
  static const float FRAMEBUFFER_OBJECT_SCALE;
  static const float ZFAR;
  static const float ZNEAR;
};


class RiftManagerApp {
protected:
  OVR::Ptr<OVR::DeviceManager> ovrManager;

public:
  RiftManagerApp() {
    ovrManager = *OVR::DeviceManager::Create();
  }
};

class RiftRenderApp : public GlfwApp, public RiftManagerApp {
public:
  struct PerEyeArgs {
    const RiftRenderApp & renderApp;
    const bool left;
    const bool right;

    glm::ivec2 viewportPosition;
    glm::mat4 strabsimusCorrection;
    glm::mat4 projectionOffset;
    glm::mat4 modelviewOffset;
    float lensOffset;

    PerEyeArgs(const RiftRenderApp & renderApp, bool left)
      : renderApp(renderApp), left(left), right(!left), lensOffset(0)
    { }

    void viewport() const {
      gl::viewport(viewportPosition, renderApp.eyeSize);
    }

    void bindUniforms(gl::ProgramPtr & program) const {
      program->setUniform("LensOffset", lensOffset);
      GL_CHECK_ERROR;
    }

    glm::mat4 getProjection() const {
      return projectionOffset * renderApp.getProjection();
    }
  };

protected:
  glm::ivec2 hmdNativeResolution;
  glm::ivec2 hmdDesktopPosition;
  GLFWmonitor * hmdMonitor;
  glm::ivec2 eyeSize;
  bool fullscreen;

  float eyeAspect;
  float eyeAspectInverse;
  float fov;
  float postDistortionScale;
  glm::vec4 distortion;
  std::array<PerEyeArgs, 2> eye;

public:
  RiftRenderApp(bool fullscreen = false);

  virtual void createRenderingTarget() {
    if (fullscreen) {
      this->createFullscreenWindow(hmdNativeResolution, hmdMonitor);
    } else {
      const GLFWvidmode * videoMode = glfwGetVideoMode(hmdMonitor);
      glfwWindowHint(GLFW_DECORATED, 0);
      createWindow(glm::ivec2(videoMode->width, videoMode->height), hmdDesktopPosition);
      if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
        FAIL("Unable to create undecorated window");
      }
    }
    eyeSize = windowSize;
    eyeSize.x /= 2;
    eye[1].viewportPosition = glm::ivec2(eyeSize.x, 0);
  }

  void bindUniforms(gl::ProgramPtr & program) const {
    program->setUniform("Aspect", eyeAspect);
    program->setUniform("PostDistortionScale", postDistortionScale);
    program->setUniform("K", distortion);
  }

  template<GLenum TextureType, GLenum TextureFormat>
  void generateDisplacementTexture(std::shared_ptr<gl::Texture<TextureType, TextureFormat> > & texture, float scale) const {
    typedef gl::Texture<TextureType, TextureFormat> Texture;
    typedef std::shared_ptr<gl::Texture<TextureType, TextureFormat> > TexturePtr;

    if (!texture) {
      texture = TexturePtr(new Texture());
    }
    texture->bind();

    glm::ivec2 textureSize(glm::vec2(eyeSize) * scale);
    texture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    texture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    texture->storage2d(textureSize);
    gl::Texture2d::unbind();

    // Create a framebuffer for the target map
    gl::TFrameBufferWrapper<GL_RG16> frameBuffer;
    frameBuffer.color = texture;
    frameBuffer.init(textureSize);
    frameBuffer.activate();
    // Rendering to the frame buffer
    {
      glDisable(GL_DEPTH_TEST);
      glClear(GL_COLOR_BUFFER_BIT);

      gl::ProgramPtr program = GlUtils::getProgram(
        Resource::SHADERS_VERTEXTORIFT_VS,
        Resource::SHADERS_GENERATEDISPLACEMENTMAP_FS);
      program->use();
      bindUniforms(program);
      program->checkConfigured();

      gl::GeometryPtr quadGeometry = GlUtils::getQuadGeometry();
      quadGeometry->bindVertexArray();
      quadGeometry->draw();
      gl::VertexArray::unbind();
      gl::Program::clear();
    }
    frameBuffer.deactivate();
    GL_CHECK_ERROR;
  }

  glm::mat4 getProjection() const {
    return glm::perspective(fov, eyeAspect, Rift::ZNEAR, Rift::ZFAR);
  }
};


class RiftApp : public RiftRenderApp {
protected:
  OVR::Util::Render::StereoMode renderMode;
  OVR::SensorFusion sensorFusion;
  OVR::Ptr<OVR::SensorDevice> ovrSensor;
  gl::TimeQueryPtr query;
  gl::Texture<GL_TEXTURE_2D, GL_RG16>::Ptr offsetTexture;
  gl::GeometryPtr quadGeometry;
  gl::FrameBufferWrapper frameBuffer;
  gl::ProgramPtr distortProgram;

  static const Resource DISTORTION_VERTEX_SHADER;
  static const Resource DISTORTION_FRAGMENT_SHADER;

public:

  void renderStringAt(const std::string & str, float x, float y);


  RiftApp();
  virtual ~RiftApp();
  virtual void createRenderingTarget();
  virtual void initGl();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void draw();

  virtual void renderScene(const PerEyeArgs & eyeArgs) {
    gl::Stacks::projection().top() = eyeArgs.getProjection();
    renderScene(eyeArgs.modelviewOffset);
  }

  // This method should be overridden in derived classes in order to render
  // the scene.  The idea FoV
  virtual void renderScene(const glm::mat4 & modelviewOffset = glm::mat4());
};

// Combine some macros together to create a single macro
// to launch a class containing a run method
#define RUN_OVR_APP(AppClass) \
    MAIN_DECL { \
        OVR::System::Init(); \
        try { \
            return AppClass().run(); \
        } catch (std::exception & error) { \
            SAY_ERR(error.what()); \
        } catch (std::string & error) { \
            SAY_ERR(error.c_str()); \
        } \
        OVR::System::Destroy(); \
        return -1; \
    }

