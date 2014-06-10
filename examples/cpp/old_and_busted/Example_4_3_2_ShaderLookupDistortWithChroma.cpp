#include "Common.h"

typedef gl::Texture<GL_TEXTURE_2D, GL_RGBA16F> LookupTexture;
typedef LookupTexture::Ptr LookupTexturePtr;

template <class Function, class SizeType = size_t>
void for_each_pixel_with_offset(SizeType size_x, SizeType size_y, Function f) {
  for (SizeType y = 0; y < size_y; ++y) {
    for (SizeType x = 0; x < size_x; ++x) {
      size_t offset = (y * size_x) + x;
      f(x, y, offset);
    }
  }
}

template <class Function>
void for_each_pixel_with_offset(const glm::uvec2 & size, Function f) {
  for_each_pixel_with_offset(size.x, size.y, f);
}

class DistortionTextureGenerator {
  glm::vec4 K;
  glm::vec2 chromaK[2];
  float lensOffset{ 0 };
  float eyeAspect{ 1.0 };
  float distortionScale;
public:

  DistortionTextureGenerator(const OVR::HMDInfo & ovrHmdInfo) {
    OVR::Util::Render::StereoConfig stereoConfig;
    stereoConfig.SetHMDInfo(ovrHmdInfo);
    const OVR::Util::Render::DistortionConfig & distortion =
      stereoConfig.GetDistortionConfig();

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

    lensOffset = distortion.XCenterOffset;
    eyeAspect = ((float)ovrHmdInfo.HResolution / 2.0f) /
      (float)ovrHmdInfo.VResolution;
  }

  glm::vec4 findUndistortedTextureCoords(StereoEye eye, const glm::vec2 & texCoord) {
    // In order to calculate the distortion, we need to know
    // the distance between the lens axis point, and the point
    // we are rendering.  So we have to move the coordinate
    // from texture space (which has a range of [0, 1]) into
    // screen space
    glm::vec2 screenCoord = (texCoord * 2.0f) - 1.0f;

    // Moving it into screen space isn't enough though.  We
    // actually need it in rift space.  Rift space differs
    // from screen space in two ways.  First, it has
    // homogeneous axis scales, so we need to correct for any
    // difference between the X and Y viewport sizes by scaling
    // the y axis by the aspect ratio
    glm::vec2 riftCoord(screenCoord.x, screenCoord.y / eyeAspect);

    // The second way rift space differs is that the origin
    // is at the intersection of the display pane with the
    // lens axis.  So we have to offset the X coordinate to
    // account for that.
    riftCoord.x += (eye == LEFT) ? -lensOffset : lensOffset;

    // Although I said we need the distance between the
    // origin and the texture, it turns out that getting
    // the scaling factor only requires the distance squared.
    // We could get the distance and then square it, but that
    // would be a waste since getting the distance in the
    // first place requires doing a square root.
    float rSq = glm::length2(riftCoord);
    float distortionScale = K[0] + rSq * (K[1] + rSq * (K[2] + rSq * K[3]));
    
    float distortionScaleBlue = distortionScale * chromaK[1][0] + chromaK[1][1] * rSq;

    glm::vec4 result;
    for (int i = 0; i < 2; ++i) {
      float distortionScaleChroma = distortionScale * chromaK[i][0] + chromaK[i][1] * rSq;
      const glm::vec2 undistortedRiftCoord = riftCoord * distortionScaleChroma;
      // Now that we've applied the distortion scale, we
      // need to reverse all the work we did to move from
      // texture space into Rift space.  So we apply the
      // inverse operations in reverse order.
      glm::vec2 undistortedScreenCoord(undistortedRiftCoord);
      undistortedScreenCoord.x -= (eye == LEFT) ? -lensOffset : lensOffset;
      undistortedScreenCoord.y *= eyeAspect;

      const glm::vec2 undistortedTexCoord = (undistortedScreenCoord + 1.0f) / 2.0f;
      result[(i * 2) + 0] = undistortedTexCoord.x;
      result[(i * 2) + 1] = undistortedTexCoord.y;
    }
    return result;
  }


  LookupTexturePtr createLookupTexture(StereoEye eye, const glm::uvec2 & lookupTextureSize) {
    std::vector<float> lookupData;
    lookupData.resize(lookupTextureSize.x * lookupTextureSize.y * 4);

    // The texture coordinates are actually from the center of the pixel, so thats what we need to use for the calculation.
    glm::vec2 texCenterOffset = glm::vec2(0.5f) / glm::vec2(lookupTextureSize);

    for_each_pixel_with_offset(lookupTextureSize, [&](size_t x, size_t y, size_t _offset) {
      // We store two floats for each pixel, so we need to double the offset
      size_t offset = _offset * 4;
      // Convert from pixels to texture coordinates
      glm::vec2 texCoord = (glm::vec2(x, y) / glm::vec2(lookupTextureSize)) + texCenterOffset;
      // Convert from undistorted texture coordinates to distorted texture coordinates
      glm::vec4 sceneTexCoord = findUndistortedTextureCoords(eye, texCoord);
      lookupData[offset] = sceneTexCoord.x;
      lookupData[offset + 1] = sceneTexCoord.y;
      lookupData[offset + 2] = sceneTexCoord.z;
      lookupData[offset + 3] = sceneTexCoord.w;
    });

    LookupTexturePtr outTexture(new LookupTexture());
    outTexture->bind();
    outTexture->image2d(lookupTextureSize, &lookupData[0], 0, GL_RGBA, GL_FLOAT);
    outTexture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    outTexture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    outTexture->parameter(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    outTexture->parameter(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    return outTexture;
  }
};

std::map<StereoEye, Resource> SCENE_IMAGES = {
  { LEFT, Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG },
  { RIGHT, Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG }
};

class ShaderLookupDistortionExample : public RiftGlfwApp {
protected:
  glm::uvec2 lookupTextureSize{ 64, 64 };
  bool chroma = true;
  std::map<StereoEye, gl::Texture2dPtr> sceneTextures;
  std::map<StereoEye, LookupTexturePtr> lookupTextures;

  gl::GeometryPtr quadGeometry;

public:
  ShaderLookupDistortionExample() {
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) switch (key) {
    case GLFW_KEY_C:
      chroma = !chroma;
      return;
    }
    RiftGlfwApp::onKey(key, scancode, action, mods);
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 1.1f, 0.1f, 1.0f);

    quadGeometry = GlUtils::getQuadGeometry();
    quadGeometry->bindVertexArray();

    DistortionTextureGenerator generator(ovrHmdInfo);
    // Load scene textures and generate lookup textures
    for_each_eye([&](StereoEye eye) {
      sceneTextures[eye] = GlUtils::getImageAsTexture(SCENE_IMAGES[eye]);
      lookupTextures[eye] = generator.createLookupTexture(eye, lookupTextureSize);
    });
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    for_each_eye([&](StereoEye eye) {
      renderEye(eye);
    });
  }

  void renderEye(StereoEye eye) {
    viewport(eye);
    gl::ProgramPtr distortProgram = GlUtils::getProgram(
      Resource::SHADERS_TEXTURED_VS,
      chroma ?
      Resource::SHADERS_RIFTCHROMAWARP_FS :
      Resource::SHADERS_RIFTWARP_FS);
    distortProgram->use();
    distortProgram->setUniform1i("Scene", 1);
    distortProgram->setUniform1i("OffsetMap", 0);

    glActiveTexture(GL_TEXTURE0);
    lookupTextures[eye]->bind();
    glActiveTexture(GL_TEXTURE1);
    sceneTextures[eye]->bind();

    quadGeometry->bindVertexArray();
    quadGeometry->draw();

    gl::VertexArray::unbind();
    glActiveTexture(GL_TEXTURE1);
    gl::Texture2d::unbind();
    glActiveTexture(GL_TEXTURE0);
    gl::Texture2d::unbind();
    gl::Program::clear();
  }
};

RUN_OVR_APP(ShaderLookupDistortionExample)
