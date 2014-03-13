#include "Common.h"

const Resource SCENE_IMAGES[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG
};

#define DISTORTION_TIMING 1
class ShaderLookupDistort : public RiftGlfwApp {
protected:
  typedef gl::Texture<GL_TEXTURE_2D, GL_RGBA16F> LookupTexture;
  typedef LookupTexture::Ptr LookupTexturePtr;

  glm::uvec2 lookupTextureSize;
  gl::Texture2dPtr sceneTextures[2];
  LookupTexturePtr lookupTextures[2];
  gl::GeometryPtr quadGeometry;
  glm::vec4 K;
  glm::vec2 chromaK[2];
  float lensOffset;
  bool chroma;

public:
  ShaderLookupDistort() : lookupTextureSize(512, 512), chroma(true) {
    OVR::Util::Render::StereoConfig stereoConfig;
    stereoConfig.SetHMDInfo(ovrHmdInfo);
    const OVR::Util::Render::DistortionConfig & distortion = 
      stereoConfig.GetDistortionConfig();

    // The Rift examples use a post-distortion scale to resize the
    // image upward after distorting it because their K values have
    // been chosen such that they always result in a scale > 1.0, and
    // thus shrink the image.  However, we can correct for that by
    // finding the distortion scale the same way the OVR examples do,
    // and then pre-multiplying the constants by it.
    double postDistortionScale = 1.0 / stereoConfig.GetDistortionScale();
    for (int i = 0; i < 4; ++i) {
      K[i] = (float)(distortion.K[i] * postDistortionScale);
    }
    // red channel correction
    chromaK[0] = glm::vec2(
        ovrHmdInfo.ChromaAbCorrection[0],
        ovrHmdInfo.ChromaAbCorrection[1]);
    // blue channel correction
    chromaK[1] = glm::vec2(
        ovrHmdInfo.ChromaAbCorrection[2],
        ovrHmdInfo.ChromaAbCorrection[3]);
    for (int i = 0; i < 2; ++i) {
      chromaK[i][0] = ((1.0f - chromaK[i][0]) * postDistortionScale) + 1.0f;
      chromaK[i][1] *= postDistortionScale;
    }
    lensOffset = 1.0f - (2.0f *
      ovrHmdInfo.LensSeparationDistance /
      ovrHmdInfo.HScreenSize);
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) switch (key) {
    case GLFW_KEY_C:
      chroma = !chroma;
    }
  }

  
  glm::vec4 findSceneTextureCoords(int eyeIndex, glm::vec2 texCoord) {
    static bool init = false;
    if (!init) {
      init = true;
    }
    texCoord *= 2.0;
    texCoord -= 1.0;
    texCoord.y /= eyeAspect;
    texCoord.x += (eyeIndex == 0) ? -lensOffset : lensOffset;

    float rSq = glm::length2(texCoord);
    float distortionScale = K[0] + rSq * (K[1] + rSq * (K[2] + rSq * K[3]));
    texCoord *= distortionScale;
    glm::vec2 chromaTexCoords[2];
    for (int i = 0; i < 2; ++i) {
      chromaTexCoords[i] = texCoord * (chromaK[i][0] + chromaK[i][1] * rSq);
      chromaTexCoords[i].x -= (eyeIndex == 0) ? -lensOffset : lensOffset;;
      chromaTexCoords[i].y *= eyeAspect;
      chromaTexCoords[i] += 1.0;
      chromaTexCoords[i] /= 2.0;
    }

    return glm::vec4(chromaTexCoords[0], chromaTexCoords[1]);
  }

  void createLookupTexture(LookupTexturePtr & outTexture, int eyeIndex) {
    size_t lookupDataSize = lookupTextureSize.x * lookupTextureSize.y * 4;
    float * lookupData = new float[lookupDataSize];
    // The texture coordinates are actually from the center of the pixel, so thats what we need to use for the calculation.
    glm::vec2 texCenterOffset = glm::vec2(0.5f) / glm::vec2(lookupTextureSize);
    size_t rowSize = lookupTextureSize.x * 4;
    for (int y = 0; y < lookupTextureSize.y; ++y) {
      for (int x = 0; x < lookupTextureSize.x; ++x) {
        size_t offset = (y * rowSize) + (x * 4);
        glm::vec2 texCoord = (glm::vec2(x, y) / glm::vec2(lookupTextureSize)) + texCenterOffset;
        glm::vec4 sceneTexCoord = findSceneTextureCoords(eyeIndex, texCoord);
        memcpy(&lookupData[offset], &sceneTexCoord[0], sizeof(float)* 4);
      }
    }

    outTexture = LookupTexturePtr(new LookupTexture());
    outTexture->bind();
    outTexture->image2d(lookupTextureSize, lookupData, 0, GL_RGBA, GL_FLOAT);
    outTexture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    outTexture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    outTexture->parameter(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    outTexture->parameter(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    delete[] lookupData;
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    quadGeometry = GlUtils::getQuadGeometry();

    // Generate the lookup textures and load the scene textures
    for (int eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
      GlUtils::getImageAsTexture(sceneTextures[eyeIndex], SCENE_IMAGES[eyeIndex]);
      createLookupTexture(lookupTextures[eyeIndex], eyeIndex);
    }
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (int eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
      renderEye(eyeIndex);
    }
    GL_CHECK_ERROR;
  }

  void renderEye(int eyeIndex) {
    viewport(eyeIndex);
    gl::ProgramPtr distortProgram = GlUtils::getProgram(
      Resource::SHADERS_TEXTURED_VS,
      chroma ? 
        Resource::SHADERS_RIFTCHROMAWARP_FS : 
        Resource::SHADERS_RIFTWARP_FS);
    distortProgram->use();
    distortProgram->setUniform1i("Scene", 1);
    distortProgram->setUniform1i("OffsetMap", 0);

    glActiveTexture(GL_TEXTURE1);
    sceneTextures[eyeIndex]->bind();
    glActiveTexture(GL_TEXTURE0);
    lookupTextures[eyeIndex]->bind();

    quadGeometry->bindVertexArray();
#ifdef DISTORTION_TIMING
    query->begin();
#endif
    quadGeometry->draw();

#ifdef DISTORTION_TIMING
    query->end();
    static long accumulator = 0;
    static long count = 0;
    accumulator += query->getReult();
    count++;
    SAY("%d ns", accumulator / count);
#endif

    gl::VertexArray::unbind();
    gl::Texture2d::unbind();
    gl::Program::clear();
  }
};

RUN_OVR_APP(ShaderLookupDistort)
