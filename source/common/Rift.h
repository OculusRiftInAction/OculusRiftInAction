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
#include "OVR_Profile.h"

class Rift {
public:
  static void getDk1HmdValues(OVR::HMDInfo & hmdInfo) {
    hmdInfo.HResolution = 1280;
    hmdInfo.VResolution = 800;
    hmdInfo.HScreenSize = 0.14976f;
    hmdInfo.VScreenSize = 0.09360f;
    hmdInfo.VScreenCenter = 0.04680f;
    hmdInfo.EyeToScreenDistance = 0.04100f;
    hmdInfo.LensSeparationDistance = 0.06350f;
    hmdInfo.InterpupillaryDistance = 0.06400f;
    hmdInfo.DistortionK[0] = 1;
    // hmdInfo.DistortionK[1] = 0.18007f;
    hmdInfo.DistortionK[1] = 0.22f;
    // hmdInfo.DistortionK[2] = 0.11502f;
    hmdInfo.DistortionK[2] = 0.24f;
    hmdInfo.DistortionK[3] = 0;
    hmdInfo.DesktopX = 100;
    hmdInfo.DesktopY = 100;
    hmdInfo.ChromaAbCorrection[0] = 0.99600f;
    hmdInfo.ChromaAbCorrection[1] = -0.00400f;
    hmdInfo.ChromaAbCorrection[2] = 1.01400f;
    hmdInfo.ChromaAbCorrection[3] = 0;
  }

  static glm::quat getStrabismusCorrection(const char * profileName = nullptr) {
    OVR::Ptr<OVR::ProfileManager> profileManager =
          *OVR::ProfileManager::Create();
    OVR::Ptr<OVR::Profile> profile;
    glm::quat result;
//    if (nullptr != profileName) {
//      profile = *profileManager->LoadProfile(profileName);
//    } else {
//      profile = *profileManager->LoadProfile(
//          profileManager->GetDefaultProfileName());
//    }
//    if (profile) {
//      OVR::Quatf strabCorrect = profile->GetStrabismusCorrection();
//      result = fromOvr(strabCorrect);
//    }
    return result;
  }

  static void getHmdInfo(
    const OVR::Ptr<OVR::DeviceManager> & ovrManager,
    OVR::HMDInfo & out) {
    if (!ovrManager) {
      FAIL("Unable to create Rift device manager");
    }

    OVR::Ptr<OVR::HMDDevice> ovrHmd = *ovrManager->
        EnumerateDevices<OVR::HMDDevice>().CreateDevice();

    if (!ovrHmd) {
//      FAIL("Unable to detect Rift display");
      getDk1HmdValues(out);
      return;
    }

    ovrHmd->GetDeviceInfo(&out);

    // Hack to fix my broken rift, reporting the wrong K
#ifdef BRADS_BROKEN_RIFT
    out.DistortionK[0] = 1;
    out.DistortionK[1] = 0.22f;
    out.DistortionK[2] = 0.24f;
    out.DistortionK[3] = 0;
#endif

    ovrHmd = nullptr;
  }

  // Fetch a glm style quaternion from an OVR sensor fusion object
  static inline glm::quat getQuaternion(OVR::SensorFusion & sensorFusion) {
    return glm::quat(getEulerAngles(sensorFusion));
  }

  // Fetch a glm vector containing Euler angles from an OVR sensor fusion object
  static inline glm::vec3 getEulerAngles(OVR::SensorFusion & sensorFusion) {
    return getEulerAngles(sensorFusion.GetPredictedOrientation());
  }

  static std::string formatOvrVector(const OVR::Vector3f & vector) {
    static char BUFFER[128];
    sprintf(BUFFER, "%+03.2f %+03.2f %+03.2f", vector.x, vector.y, vector.z);
    return std::string(BUFFER);
  }

  static glm::vec3 fromOvr(const OVR::Vector3f & in) {
    return glm::vec3(in.x, in.y, in.z);
  }

  static glm::quat fromOvr(const OVR::Quatf & in) {
    return glm::quat(getEulerAngles(in));
  }

  static glm::vec3 getEulerAngles(const OVR::Quatf & in) {
    glm::vec3 eulerAngles;
    in.GetEulerAngles<
        OVR::Axis_X, OVR::Axis_Y, OVR::Axis_Z,
        OVR::Rotate_CW, OVR::Handed_R
        >(&eulerAngles.x, &eulerAngles.y, &eulerAngles.z);
    return eulerAngles;
  }

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
  glm::ivec2 hmdSize;
  glm::ivec2 hmdDesktopPosition;
  glm::ivec2 eyeSize;

  float eyeAspect;
  float fov;
  float postDistortionScale;
  glm::vec4 distortion;
  std::array<PerEyeArgs, 2> eye;

public:
  RiftRenderApp() :
    eyeAspect(1),
    fov(60),
    postDistortionScale(1),
    eye({ PerEyeArgs(*this, true), PerEyeArgs(*this, false) }) {

    OVR::HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    hmdSize = glm::ivec2(hmdInfo.HResolution, hmdInfo.VResolution);
    hmdDesktopPosition = glm::ivec2(hmdInfo.DesktopX, hmdInfo.DesktopY);
    eyeSize = glm::ivec2(hmdInfo.HResolution / 2, hmdInfo.VResolution);
    eyeAspect = glm::aspect(eyeSize);
    eye[1].viewportPosition = glm::ivec2(eyeSize.x, 0);
    float lensDistance =
      hmdInfo.LensSeparationDistance /
      hmdInfo.HScreenSize;
    eye[0].lensOffset = 1.0f - (2.0f * lensDistance);
    eye[1].lensOffset = -eye[0].lensOffset;
    distortion = glm::vec4(
      hmdInfo.DistortionK[0],
      hmdInfo.DistortionK[1],
      hmdInfo.DistortionK[2],
      hmdInfo.DistortionK[3]);

    OVR::Util::Render::StereoConfig stereoConfig;
    stereoConfig.SetHMDInfo(hmdInfo);
    fov = stereoConfig.GetYFOVDegrees();
    postDistortionScale = 1.0f / stereoConfig.GetDistortionScale();

    {
      float projectionOffset = stereoConfig.GetProjectionCenterOffset();
      glm::vec3 v = glm::vec3(projectionOffset, 0, 0);
      eye[0].projectionOffset = glm::translate(glm::mat4(), v);
      eye[1].projectionOffset = glm::translate(glm::mat4(), -v);
    }

    {
      float modelviewOffset = stereoConfig.GetIPD() / 2.0f;
      glm::vec3 v = glm::vec3(modelviewOffset, 0, 0);

      glm::quat strabismusCorrection = Rift::getStrabismusCorrection();
      eye[0].modelviewOffset =
          glm::translate(glm::mat4(), v)
        *
      glm::mat4_cast(strabismusCorrection)
        ;
      eye[1].modelviewOffset =
          glm::translate(glm::mat4(), -v)
      *
      glm::mat4_cast(glm::inverse(strabismusCorrection))
          ;
      //    profile->SetStrabismusCorrection(OVR::Quatf(
      //        -0.0195252169,
      //        0.0247310139,
      //        -0.0144390352,
      //        0.998797238
      //    ));
    }


  }

  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);
    createWindow(hmdSize, hmdDesktopPosition);
    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
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
        } catch (const std::string & error) { \
            SAY(error.c_str()); \
        } \
        OVR::System::Destroy(); \
        return -1; \
    }

