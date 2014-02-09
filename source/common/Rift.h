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

class Rift {
public:
  static void getDk1HmdValues(OVR::HMDInfo & hmdInfo);
  static void getRiftPositionAndSize(const OVR::HMDInfo & hmdInfo,
      glm::ivec2 & windowPosition, glm::uvec2 & windowSize);

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

typedef gl::Texture<GL_TEXTURE_2D, GL_RG16F> RiftLookupTexture;
typedef RiftLookupTexture::Ptr RiftLookupTexturePtr;

class RiftDistortionHelper {
  glm::dvec4 K;
  double lensOffset;
  double eyeAspect;

  double getLensOffset(int eyeIndex) const{
    return (eyeIndex == 0) ? -lensOffset : lensOffset;
  }

  static glm::dvec2 screenToTexture(const glm::dvec2 & v) {
    return ((v + 1.0) / 2.0);
  }

  static glm::dvec2 textureToScreen(const glm::dvec2 & v) {
    return ((v * 2.0) - 1.0);
  }

  glm::dvec2 screenToRift(const glm::dvec2 & v, int eyeIndex) const{
    return glm::dvec2(v.x + getLensOffset(eyeIndex), v.y / eyeAspect);
  }

  glm::dvec2 riftToScreen(const glm::dvec2 & v, int eyeIndex) const{
    return glm::dvec2(v.x - getLensOffset(eyeIndex), v.y * eyeAspect);
  }

  glm::dvec2 textureToRift(const glm::dvec2 & v, int eyeIndex) const {
    return screenToRift(textureToScreen(v), eyeIndex);
  }

  glm::dvec2 riftToTexture(const glm::dvec2 & v, int eyeIndex) const{
    return screenToTexture(riftToScreen(v, eyeIndex));
  }

  double getUndistortionScaleForRadiusSquared(double rSq) const {
    double distortionScale = K[0]
      + rSq * (K[1] + rSq * (K[2] + rSq * K[3]));
    return distortionScale;
  }

  double getUndistortionScale(const glm::dvec2 & v) const {
    double rSq = glm::length2(v);
    return getUndistortionScaleForRadiusSquared(rSq);
  }

  double getUndistortionScaleForRadius(double r) const{
    return getUndistortionScaleForRadiusSquared(r * r);
  }

  glm::dvec2 getUndistortedPosition(const glm::dvec2 & v) const {
    return v * getUndistortionScale(v);
  }

  glm::dvec2 getTextureLookupValue(const glm::dvec2 & texCoord, int eyeIndex) const {
    glm::dvec2 riftPos = textureToRift(texCoord, eyeIndex);
    glm::dvec2 distorted = getUndistortedPosition(riftPos);
    glm::dvec2 result = riftToTexture(distorted, eyeIndex);
    return result;
  }

  bool closeEnough(double a, double b, double epsilon = 1e-5) const {
    return abs(a - b) < epsilon;
  }

  double getDistortionScaleForRadius(double rTarget) const {
    double max = rTarget * 2;
    double min = 0;
    double distortionScale;
    while (true) {
      double rSource = ((max - min) / 2.0) + min;
      distortionScale = getUndistortionScaleForRadiusSquared(
        rSource * rSource);
      double rResult = distortionScale * rSource;
      if (closeEnough(rResult, rTarget)) {
        break;
      }
      if (rResult < rTarget) {
        min = rSource;
      }
      else {
        max = rSource;
      }
    }
    return 1.0 / distortionScale;
  }

  glm::dvec2 findDistortedVertexPosition(const glm::dvec2 & source,
    int eyeIndex) const {
    const glm::dvec2 rift = screenToRift(source, eyeIndex);
    double rTarget = glm::length(rift);
    double distortionScale = getDistortionScaleForRadius(rTarget);
    glm::dvec2 result = rift * distortionScale;
    glm::dvec2 resultScreen = riftToScreen(result, eyeIndex);
    return resultScreen;
  }

public:
  RiftDistortionHelper(const OVR::HMDInfo & hmdInfo) {
    OVR::Util::Render::DistortionConfig Distortion;
    OVR::Util::Render::StereoConfig stereoConfig;
    stereoConfig.SetHMDInfo(hmdInfo);
    Distortion = stereoConfig.GetDistortionConfig();

    // The Rift examples use a post-distortion scale to resize the
    // image upward after distorting it because their K values have
    // been chosen such that they always result in a scale > 1.0, and
    // thus shrink the image.  However, we can correct for that by
    // finding the distortion scale the same way the OVR examples do,
    // and then pre-multiplying the constants by it.
    double postDistortionScale = 1.0 / stereoConfig.GetDistortionScale();
    for (int i = 0; i < 4; ++i) {
      K[i] = Distortion.K[i] * postDistortionScale;
    }
    lensOffset = 1.0f
      - (2.0f * hmdInfo.LensSeparationDistance / hmdInfo.HScreenSize);
    eyeAspect = hmdInfo.HScreenSize / 2.0f / hmdInfo.VScreenSize;
  }

  RiftLookupTexturePtr createLookupTexture(const glm::uvec2 & lookupTextureSize, int eyeIndex) const {
    size_t lookupDataSize = lookupTextureSize.x * lookupTextureSize.y * 2;
    float * lookupData = new float[lookupDataSize];
    // The texture coordinates are actually from the center of the pixel, so thats what we need to use for the calculation.
    glm::dvec2 texCenterOffset = glm::dvec2(0.5f) / glm::dvec2(lookupTextureSize);
    size_t rowSize = lookupTextureSize.x * 2;
    for (size_t y = 0; y < lookupTextureSize.y; ++y) {
      for (size_t x = 0; x < lookupTextureSize.x; ++x) {
        size_t offset = (y * rowSize) + (x * 2);
        glm::dvec2 texCoord = (glm::dvec2(x, y) / glm::dvec2(lookupTextureSize)) + texCenterOffset;
        glm::dvec2 riftCoord = textureToRift(texCoord, eyeIndex);
        glm::dvec2 undistortedRiftCoord = getUndistortedPosition(riftCoord);
        glm::dvec2 undistortedTexCoord = riftToTexture(undistortedRiftCoord, eyeIndex);
        lookupData[offset] = (float)undistortedTexCoord.x;
        lookupData[offset + 1] = (float)undistortedTexCoord.y;
      }
    }

    RiftLookupTexturePtr outTexture(new RiftLookupTexture());
    outTexture->bind();
    outTexture->image2d(lookupTextureSize, lookupData, 0, GL_RG, GL_FLOAT);
    outTexture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    outTexture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    outTexture->parameter(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    outTexture->parameter(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    delete[] lookupData;
    return outTexture;
  }

  gl::GeometryPtr createDistortionMesh(
    const glm::uvec2 & distortionMeshResolution, int eyeIndex) const{
    std::vector<glm::vec4> vertexData;
    vertexData.reserve(
      distortionMeshResolution.x * distortionMeshResolution.y * 2);
    // The texture coordinates are actually from the center of the pixel, so thats what we need to use for the calculation.
    for (size_t y = 0; y < distortionMeshResolution.y; ++y) {
      for (size_t x = 0; x < distortionMeshResolution.x; ++x) {
        // Create a texture coordinate that goes from [0, 1]
        glm::dvec2 texCoord = (glm::dvec2(x, y)
          / glm::dvec2(distortionMeshResolution - glm::uvec2(1)));
        // Create the vertex coordinate in the range [-1, 1]
        glm::dvec2 vertexPos = (texCoord * 2.0) - 1.0;

        // now find the distorted vertex position from the original
        // scene position
        vertexPos = findDistortedVertexPosition(vertexPos, eyeIndex);
        vertexData.push_back(glm::vec4(vertexPos, 0, 1));
        vertexData.push_back(glm::vec4(texCoord, 0, 1));
      }
    }

    std::vector<GLuint> indexData;
    for (size_t y = 0; y < distortionMeshResolution.y - 1; ++y) {
      size_t rowStart = y * distortionMeshResolution.x;
      size_t nextRowStart = rowStart + distortionMeshResolution.x;
      for (size_t x = 0; x < distortionMeshResolution.x; ++x) {
        indexData.push_back(nextRowStart + x);
        indexData.push_back(rowStart + x);
      }
      indexData.push_back(UINT_MAX);

    }
    return gl::GeometryPtr(
      new gl::Geometry(vertexData, indexData, indexData.size(),
      gl::Geometry::Flag::HAS_TEXTURE,
      GL_TRIANGLE_STRIP));
  }
};


class RiftManagerApp {
protected:
  OVR::Ptr<OVR::DeviceManager> ovrManager;

public:
  RiftManagerApp() {
    ovrManager = *OVR::DeviceManager::Create();
  }
};

/**
A class that takes care of the basic duties of putting an OpenGL
window on the desktop in the correct position so that it's visible
through the Rift.
*/
class RiftGlfwApp : public GlfwApp, public RiftManagerApp {
protected:
  GLFWmonitor * hmdMonitor;
  glm::uvec2 hmdNativeResolution;
  glm::ivec2 hmdDesktopPosition;
  glm::uvec2 eyeSize;
  const bool fullscreen;
  float eyeAspect;
  float eyeAspectInverse;

public:
  RiftGlfwApp(bool fullscreen = false) :
    fullscreen(fullscreen),
    eyeAspect(1)
  {
    OVR::HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    hmdNativeResolution = glm::ivec2(hmdInfo.HResolution, hmdInfo.VResolution);
    eyeAspect = glm::aspect(hmdNativeResolution) / 2.0f;
    eyeAspectInverse = 1.0f / eyeAspect;
    hmdDesktopPosition = glm::ivec2(hmdInfo.DesktopX, hmdInfo.DesktopY);
    hmdMonitor = GlfwApp::getMonitorAtPosition(hmdDesktopPosition);
    if (fullscreen || !hmdMonitor) {
      eyeSize = hmdNativeResolution;
    }
    else {
      const GLFWvidmode * videoMode = glfwGetVideoMode(hmdMonitor);
      eyeSize = glm::uvec2(videoMode->width, videoMode->height);
    }
    eyeSize.x /= 2;
  }

  virtual void createRenderingTarget() {
    if (fullscreen) {
      // Fullscreen apps should use the native resolution of the Rift
      this->createFullscreenWindow(hmdNativeResolution, hmdMonitor);
    }
    else {
      const GLFWvidmode * videoMode = glfwGetVideoMode(hmdMonitor);
      glfwWindowHint(GLFW_DECORATED, 0);
      createWindow(glm::uvec2(videoMode->width, videoMode->height), hmdDesktopPosition);
      if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
        FAIL("Unable to create undecorated window");
      }
    }
  }

  virtual void viewport(int eyeIndex) {
    glm::uvec2 viewportPosition(eyeIndex == 0 ? 0 : eyeSize.x, 0);
    gl::viewport(viewportPosition, eyeSize);
  }
};

class RiftApp : public RiftGlfwApp {
public:
  struct PerEyeArg {
  private:
    const RiftApp & parent;
  public:
    const bool left;
    const bool right;
    glm::mat4 strabsimusCorrection;
    glm::mat4 projectionOffset;
    glm::mat4 modelviewOffset;
    RiftLookupTexturePtr lookupTexture;

    PerEyeArg(const RiftApp & parent, bool left)
      : parent(parent), left(left), right(!left)
    { }

    glm::mat4 getProjection() const {
      return projectionOffset * parent.getProjection();
    }
  };
protected:

  OVR::Util::Render::StereoMode renderMode;
  OVR::SensorFusion sensorFusion;
  OVR::Ptr<OVR::SensorDevice> ovrSensor;
  gl::Texture<GL_TEXTURE_2D, GL_RG16>::Ptr offsetTexture;
  gl::GeometryPtr quadGeometry;
  gl::FrameBufferWrapper frameBuffer;
  gl::ProgramPtr distortProgram;
  std::array<PerEyeArg, 2> eyes;
  float fov;

  glm::mat4 getProjection() const {
    return glm::perspective(fov, eyeAspect, Rift::ZNEAR, Rift::ZFAR);
  }

public:

  void renderStringAt(const std::string & str, float x, float y);


  RiftApp(bool fullscreen = false);
  virtual ~RiftApp();
  virtual void createRenderingTarget();
  virtual void initGl();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void draw();

  virtual void renderScene(const PerEyeArg & eyeArgs) {
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

