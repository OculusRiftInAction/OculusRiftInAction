#include "Common.h"
#include "GlTexture.h"
#include <fstream>
#include <cstdio>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class PhotoSphereExample : public RiftApp {

  const std::string filepath = "c:\\users\\alex\\desktop\\PANO_20140620_160351.jpg";

public:
  PhotoSphereExample() {
  }

  void drawSphere() {
    static gl::GeometryPtr geometry = GlUtils::getSphereGeometry();
    static gl::ProgramPtr program = GlUtils::getProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
    static gl::TexturePtr t = loadAndPositionPhotoSphereImage(filepath);

    t->bind();
    GlUtils::renderGeometry(geometry, program);
    t->unbind();
  }

  void renderScene() {
    glClear(GL_DEPTH_BUFFER_BIT);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.withPush([&]{
      mv.translate(glm::vec3(0, 0, -1)).scale(0.2f);
      drawSphere();
    });
    mv.withPush([&]{
      mv.translate(glm::vec3(0, 0, -1)).scale(0.02f);
      mv.postMultiply(headPose);
      GlUtils::renderRift();
    });
    glDisable(GL_CULL_FACE);
    mv.withPush([&]{
      mv.scale(50.0f);
      drawSphere();
    });
    glEnable(GL_CULL_FACE);
  }

  /**
   * Call out to the exiv2 binary to retrieve the XMP fields encoded in the target image file.
   */
  static gl::TexturePtr loadAndPositionPhotoSphereImage(const std::string & pathToImage) {
    glm::uvec2 fullPanoSize;
    glm::uvec2 croppedImageSize;
    glm::uvec2 croppedImagePos;
    gl::TexturePtr texture;

    cv::Mat mat = cv::imread(pathToImage.c_str());
    cv::cvtColor(mat, mat, CV_BGR2RGB);

    if (loadExifDataFromImage(pathToImage, fullPanoSize, croppedImageSize, croppedImagePos)) {

      // EXIF data parsed succesfully
      uchar *embedded = (uchar*)malloc(fullPanoSize.x * fullPanoSize.y * 3);
      insetImage(fullPanoSize, croppedImageSize, croppedImagePos, mat, embedded);
      texture = GlUtils::getImageAsTexture(fullPanoSize, embedded);
      free(embedded);
    } else {

      // Failed to load EXIF data
      texture = GlUtils::getImageAsTexture(glm::uvec2(mat.cols, mat.rows), mat.datastart);
    }
    return texture;
  }

  /**
   * Embed the image in mat into a larger frame.
   */
  static void insetImage(glm::uvec2 &fullPanoSize, glm::uvec2 &croppedImageSize, glm::uvec2 &croppedImagePos, cv::Mat &mat, uchar *out) {
    memset(out, 84, fullPanoSize.x * fullPanoSize.y * 3);
    for (unsigned int y = 0; y < croppedImageSize.y; y++) {
      int srcRow = y * croppedImageSize.x * 3;
      int destRow = (croppedImagePos.y + y) * (fullPanoSize.x * 3);
      memcpy(out + destRow + croppedImagePos.x, mat.datastart + srcRow, croppedImageSize.x * 3);
    }
  }

  static bool loadExifDataFromImage(const std::string &pathToImage, glm::uvec2 &fullPanoSize, glm::uvec2 &croppedImageSize, glm::uvec2 &croppedImagePos) {
    static const std::string pathToExiv2 = "C:\\Users\\Alex\\GitHub\\OculusRiftInAction\\libraries\\exiv2-0.24\\exiv2.exe";
    static const std::string exiv2Params = "-pa print";
    static const std::string tempFile = "temp.txt";

    // Extract XMP data from image, write to temp file
    std::string command = pathToExiv2 + " " + exiv2Params + " " + pathToImage + " > " + tempFile;
    system(command.c_str());

    // Extract pano fields from temp file
    int fieldsFound = 0;
    std::string line;
    std::ifstream infile(tempFile);
    while (std::getline(infile, line)) {
      std::istringstream iss(line);
      std::string field, type, name;
      int value;
      if (iss >> field >> type >> name >> value) {
        if (field.compare("Xmp.GPano.FullPanoWidthPixels") == 0)          { fullPanoSize.x = value; fieldsFound |= (1 << 0); }
        if (field.compare("Xmp.GPano.FullPanoHeightPixels") == 0)         { fullPanoSize.y = value; fieldsFound |= (1 << 1); }
        if (field.compare("Xmp.GPano.CroppedAreaLeftPixels") == 0)        { croppedImagePos.x = value; fieldsFound |= (1 << 2); }
        if (field.compare("Xmp.GPano.CroppedAreaTopPixels") == 0)         { croppedImagePos.y = value; fieldsFound |= (1 << 3); }
        if (field.compare("Xmp.GPano.CroppedAreaImageWidthPixels") == 0)  { croppedImageSize.x = value; fieldsFound |= (1 << 4); }
        if (field.compare("Xmp.GPano.CroppedAreaImageHeightPixels") == 0) { croppedImageSize.y = value; fieldsFound |= (1 << 5); }
      }
    }
    infile.close();

    if (fieldsFound != ((1 << 6) - 1)) {
      std::cout << "Didn't find expected XMP fields; output has been left in " + tempFile;
      return false;
    }
    else {
      remove(tempFile.c_str());
      return true;
    }
  }
};

RUN_OVR_APP(PhotoSphereExample);
