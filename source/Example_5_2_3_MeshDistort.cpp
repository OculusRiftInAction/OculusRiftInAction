#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

#define DISTORTION_TIMING 1
class DistortionHelper {
  glm::dvec4 K;
  double lensOffset;
  double eyeAspect;

  double getLensOffset(int eyeIndex) {
    return (eyeIndex == 0) ? -lensOffset : lensOffset;
  }

  static glm::dvec2 screenToTexture(const glm::dvec2 & v) {
    return ((v + 1.0) / 2.0);
  }

  static glm::dvec2 textureToScreen(const glm::dvec2 & v) {
    return ((v * 2.0) - 1.0);
  }

  glm::dvec2 screenToRift(const glm::dvec2 & v, int eyeIndex) {
    return glm::dvec2(v.x + getLensOffset(eyeIndex), v.y / eyeAspect);
  }

  glm::dvec2 riftToScreen(const glm::dvec2 & v, int eyeIndex) {
    return glm::dvec2(v.x - getLensOffset(eyeIndex), v.y * eyeAspect);
  }

  glm::dvec2 textureToRift(const glm::dvec2 & v, int eyeIndex) {
    return screenToRift(textureToScreen(v), eyeIndex);
  }

  glm::dvec2 riftToTexture(const glm::dvec2 & v, int eyeIndex) {
    return screenToTexture(riftToScreen(v, eyeIndex));
  }

  double getUndistortionScaleForRadiusSquared(double rSq) {
    double distortionScale = K[0]
        + rSq * (K[1] + rSq * (K[2] + rSq * K[3]));
    return distortionScale;
  }

  double getUndistortionScale(const glm::dvec2 & v) {
    double rSq = glm::length2(v);
    return getUndistortionScaleForRadiusSquared(rSq);
  }

  double getUndistortionScaleForRadius(double r) {
    return getUndistortionScaleForRadiusSquared(r * r);
  }

  glm::dvec2 getUndistortedPosition(const glm::dvec2 & v) {
    return v * getUndistortionScale(v);
  }

  glm::dvec2 getTextureLookupValue(const glm::dvec2 & texCoord, int eyeIndex) {
    glm::dvec2 riftPos = textureToRift(texCoord, eyeIndex);
    glm::dvec2 distorted = getUndistortedPosition(riftPos);
    glm::dvec2 result = riftToTexture(distorted, eyeIndex);
    return result;
  }

  bool closeEnough(double a, double b, double epsilon = 1e-5) {
    return abs(a - b) < epsilon;
  }

  double getDistortionScaleForRadius(double rTarget) {
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
      } else {
        max = rSource;
      }
    }
    return 1.0 / distortionScale;
  }

  glm::dvec2 findDistortedVertexPosition(const glm::dvec2 & source,
      int eyeIndex) {
    const glm::dvec2 rift = screenToRift(source, eyeIndex);
    double rTarget = glm::length(rift);
    double distortionScale = getDistortionScaleForRadius(rTarget);
    glm::dvec2 result = rift * distortionScale;
    glm::dvec2 resultScreen = riftToScreen(result, eyeIndex);
    return resultScreen;
  }

public:
  DistortionHelper(const OVR::HMDInfo & hmdInfo) {
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

  TexturePtr createLookupTexture() {
    return TexturePtr();
  }

  GeometryPtr createDistortionMesh(
      const glm::uvec2 & distortionMeshResolution, int eyeIndex) {
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
    return GeometryPtr(
        new Geometry(vertexData, indexData, indexData.size(),
            Geometry::Flag::HAS_TEXTURE,
            GL_TRIANGLE_STRIP));
  }
};

const Resource SCENE_IMAGES[2] = {
    Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG,
    Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG };

class PostProcessDistortRift : public RiftGlfwApp {
protected:
  Texture2dPtr sceneTextures[2];
  GeometryPtr distortionGeometry[2];

public:

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(UINT_MAX);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    OVR::HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    DistortionHelper distortionHelper(hmdInfo);

    for (int eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
      // load the scene textures
      GlUtils::getImageAsTexture(sceneTextures[eyeIndex], SCENE_IMAGES[eyeIndex]);
      // Generate the distortion meshes
      distortionGeometry[eyeIndex] = 
        distortionHelper.createDistortionMesh(
        glm::uvec2(64, 64), eyeIndex);
    }
  }

  void draw() {
    glClearColor(0.2f, 0.2f, 0.2f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    for (int eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
      renderEye(eyeIndex);
    }
    GL_CHECK_ERROR;
  }

  void renderEye(int eyeIndex) {
    // Enable this to see the mesh itself
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    viewport(eyeIndex);

    ProgramPtr distortProgram = GlUtils::getProgram(
      Resource::SHADERS_TEXTURED_VS,
        Resource::SHADERS_TEXTURE_FS);
    distortProgram->use();
    sceneTextures[eyeIndex]->bind();
    distortionGeometry[eyeIndex]->bindVertexArray();

#ifdef DISTORTION_TIMING
    query->begin();
#endif

    distortionGeometry[eyeIndex]->draw();

#ifdef DISTORTION_TIMING
    query->end();
    static long accumulator = 0;
    static long count = 0;
    accumulator += query->getReult();
    count++;
    SAY("%d ns", accumulator / count);
#endif

    VertexArray::unbind();
    Texture2d::unbind();
    Program::clear();
  }
};

RUN_OVR_APP(PostProcessDistortRift)
