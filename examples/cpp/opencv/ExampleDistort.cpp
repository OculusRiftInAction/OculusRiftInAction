#include "Common.h"
#include <opencv2/opencv.hpp>

const Resource SCENE_IMAGES[2] = {
    Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG,
    Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG };

cv::Mat getImage(Resource resource, bool flip = false) {
  size_t size = Resources::getResourceSize(resource);
  std::vector<uint8_t> data;
  data.resize(size);
  Resources::getResourceData(resource, &data[0]);
  cv::Mat image = cv::imdecode(data, CV_LOAD_IMAGE_COLOR);
  if (flip) {
    cv::flip(image, image, 0);
  }
  return image;
}

class ShaderLookupDistort : public RiftManagerApp {
protected:
  cv::Mat sceneTextures[2];
  std::map<StereoEye, cv::Mat> lookupMapsX;
  std::map<StereoEye, cv::Mat> lookupMapsY;
  float K[4];
  float lensOffset;

public:
  ShaderLookupDistort() {
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
      K[i] = (float) (distortion.K[i] * postDistortionScale);
    }
    lensOffset = 1.0f
        - (2.0f * ovrHmdInfo.LensSeparationDistance
            / ovrHmdInfo.HScreenSize);

    for_each_eye([&](StereoEye eye) {
      sceneTextures[eye] = getImage(SCENE_IMAGES[eye]);
      createDistortionMap(eye);
    });
  }

  glm::vec2 findSceneTextureCoords(StereoEye eye, glm::vec2 texCoord) {
    static bool init = false;
    if (!init) {
      init = true;
    }

    // In order to calculate the distortion, we need to know
    // the distance between the lens axis point, and the point
    // we are rendering.  So we have to move the coordinate
    // from texture space (which has a range of [0, 1]) into
    // screen space
    texCoord *= 2.0;
    texCoord -= 1.0;

    // Moving it into screen space isn't enough though.  We
    // actually need it in rift space.  Rift space differs
    // from screen space in two ways.  First, it has
    // homogeneous axis scales, so we need to correct for any
    // difference between the X and Y viewport sizes by scaling
    // the y axis by the aspect ratio
    texCoord.y /= eyeAspect;

    // The second way rift space differs is that the origin
    // is at the intersection of the display pane with the
    // lens axis.  So we have to offset the X coordinate to
    // account for that.
    texCoord.x += (eye == LEFT) ? -lensOffset : lensOffset;

    // Although I said we need the distance between the
    // origin and the texture, it turns out that getting
    // the scaling factor only requires the distance squared.
    // We could get the distance and then square it, but that
    // would be a waste since getting the distance in the
    // first place requires doing a square root.
    float rSq = glm::length2(texCoord);
    float distortionScale = K[0] + rSq * (K[1] + rSq * (K[2] + rSq * K[3]));
    texCoord *= distortionScale;

    // Now that we've applied the distortion scale, we
    // need to reverse all the work we did to move from
    // texture space into Rift space.  So we apply the
    // inverse operations in reverse order.
    texCoord.x -= (eye == LEFT) ? -lensOffset : lensOffset;;
    texCoord.y *= eyeAspect;
    texCoord += 1.0;
    texCoord /= 2.0;

    // Our texture coordinate is now distorted.  Or rather,
    // for the input texture coordinate on the distorted screen
    // we now have the undistorted source coordinates to pull
    // the color from
    return texCoord;
  }

  void createDistortionMap(StereoEye eye) {
    cv::Scalar defScalar((float) 1.0f, (float) 1.0f);
    cv::Mat & map_x = lookupMapsX[eye];
    map_x = cv::Mat(sceneTextures[eye].size(), CV_32FC1);
    cv::Mat & map_y = lookupMapsY[eye];
    map_y = cv::Mat(sceneTextures[eye].size(), CV_32FC1);
    const glm::vec2 imageSize(sceneTextures[eye].cols, sceneTextures[eye].rows);
    glm::vec2 texCenterOffset = glm::vec2(0.5f)
        / glm::vec2(imageSize);
    for (size_t row = 0; row < sceneTextures[eye].rows; ++row) {
      for (size_t col = 0; col < sceneTextures[eye].cols; ++col) {
        glm::vec2 textureCoord(col, row);
        textureCoord /= imageSize;
        textureCoord += texCenterOffset;
        textureCoord.y = 1 - textureCoord.y;
        glm::vec2 distortedCoord = findSceneTextureCoords(eye, textureCoord);
        distortedCoord.y = 1 - distortedCoord.y;
        distortedCoord *= imageSize;
        map_x.at<float>(row, col) = distortedCoord.x;
        map_y.at<float>(row, col) = distortedCoord.y;
      }
    }
  }

  std::array<const char *, 2> WINDOWS{ {
    "RiftLeft",
    "RiftRight"
  } };

  int run() {
    std::map<StereoEye, cv::Mat> distorted;
    for_each_eye([&](StereoEye eye) {
      cv::namedWindow(WINDOWS[eye], CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO);
      cvResizeWindow(WINDOWS[eye], 640, 800);
      cvMoveWindow(WINDOWS[eye], eye == LEFT ? 100 : 800, 100);
      distorted[eye] = cv::Mat(640, 800, sceneTextures[eye].type());
    });

    do {
      for_each_eye([&](StereoEye eye) {
        cv::remap(sceneTextures[eye], distorted[eye], lookupMapsX[eye], lookupMapsY[eye], CV_INTER_LINEAR);
        cv::imshow(WINDOWS[eye], distorted[eye]);     // Show our image inside it.
      });
    } while (27 != (0xff & cv::waitKey(15)));
    return 0;
  }
};

RUN_OVR_APP(ShaderLookupDistort)

