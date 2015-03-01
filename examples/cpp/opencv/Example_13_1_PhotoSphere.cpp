#include "Common.h"
#include <fstream>
#include <cstdio>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


class PhotoSphereExample : public RiftApp {

public:
  PhotoSphereExample() {
  }

  void drawSphere() {
    static ProgramPtr program = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
    static ShapeWrapperPtr geometry = oria::loadShape({ "Position", "TexCoord" }, Resource::MESHES_SPHERE_CTM, program);
    static TexturePtr t = loadAndPositionPhotoSphereImage(Resource::IMAGES_PANO_20140620_160351_JPG);
    Platform::addShutdownHook([]{
      program.reset();
      geometry.reset();
      t.reset();
    }); 

    t->Bind(oglplus::Texture::Target::_2D);
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&] {
      // Invert the sphere to see its insides
      mv.scale(vec3(-1));
      oria::renderGeometry(geometry, program);
    });
    oglplus::DefaultTexture().Bind(oglplus::Texture::Target::_2D);
  }

  void renderScene() {
    using namespace oglplus;
    Context::Clear().DepthBuffer();
    glClear(GL_DEPTH_BUFFER_BIT);

    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&]{
      mv.translate(glm::vec3(0, 0, -1)).scale(0.2f);
      drawSphere();
    });
    mv.withPush([&]{
      mv.translate(glm::vec3(0, 0, -1)).scale(0.02f);
      mv.postMultiply(ovr::toGlm(getEyePose()));
      oria::renderRift();
    });

    Context::Disable(Capability::CullFace);
    glDisable(GL_CULL_FACE);
    mv.withPush([&]{
      mv.scale(50.0f);
      drawSphere();
    });
    Context::Enable(Capability::CullFace);
  }

  /**
   * Call out to the exiv2 binary to retrieve the XMP fields encoded in the target image file.
   */
  static TexturePtr loadAndPositionPhotoSphereImage(Resource res) {
    glm::uvec2 fullPanoSize;
    glm::uvec2 croppedImageSize;
    glm::uvec2 croppedImagePos;
    using namespace oglplus;

    TexturePtr texture(new Texture());
    Platform::addShutdownHook([&]{
      texture.reset();
    });
    auto v = Platform::getResourceByteVector(res);
    cv::Mat mat = cv::imdecode(v, CV_LOAD_IMAGE_COLOR);
    cv::cvtColor(mat, mat, CV_BGR2RGB);
    //cv::flip(mat, mat, 0);
    Context::Bound(TextureTarget::_2D, *texture)
      .MagFilter(TextureMagFilter::Linear)
      .MinFilter(TextureMinFilter::Linear);

    if (parseExifData(Platform::getResourceString(Resource::MISC_PANO_20140620_160351_EXIV), fullPanoSize, croppedImageSize, croppedImagePos)) {

      // EXIF data parsed succesfully
      uchar *embedded = (uchar*)malloc(fullPanoSize.x * fullPanoSize.y * 3);
      insetImage(fullPanoSize, croppedImageSize, croppedImagePos, mat, embedded);
      Context::Bound(TextureTarget::_2D, *texture)
        .Image2D(images::Image(fullPanoSize.x, fullPanoSize.y, 1, 3, embedded));
      free(embedded);
    }
    else {
      // Failed to load EXIF data
      texture->Bind(Texture::Target::_2D);
      Context::PixelStore(PixelStorageMode::UnpackAlignment, 1);
      Context::Bound(TextureTarget::_2D, *texture)
        .Image2D(images::Image(mat.cols, mat.rows, 1, 3, mat.datastart));
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

  static bool parseExifData(const std::string & exifData, glm::uvec2 &fullPanoSize, glm::uvec2 &croppedImageSize, glm::uvec2 &croppedImagePos) {
    // Extract pano fields from temp file
    int fieldsFound = 0;
    std::string line;
    std::istringstream infile(exifData);
    while (std::getline(infile, line)) {
      std::istringstream iss(line);
      std::string field, type, name;
      int value;
      if (iss >> field >> type >> name >> value) {
        if (field.compare("Xmp.GPano.FullPanoWidthPixels") == 0) { fullPanoSize.x = value; fieldsFound |= (1 << 0); }
        if (field.compare("Xmp.GPano.FullPanoHeightPixels") == 0) { fullPanoSize.y = value; fieldsFound |= (1 << 1); }
        if (field.compare("Xmp.GPano.CroppedAreaLeftPixels") == 0) { croppedImagePos.x = value; fieldsFound |= (1 << 2); }
        if (field.compare("Xmp.GPano.CroppedAreaTopPixels") == 0) { croppedImagePos.y = value; fieldsFound |= (1 << 3); }
        if (field.compare("Xmp.GPano.CroppedAreaImageWidthPixels") == 0) { croppedImageSize.x = value; fieldsFound |= (1 << 4); }
        if (field.compare("Xmp.GPano.CroppedAreaImageHeightPixels") == 0) { croppedImageSize.y = value; fieldsFound |= (1 << 5); }
      }
    }
    if (fieldsFound != ((1 << 6) - 1)) {
      std::cout << "Didn't find expected XMP fields" << std::endl;
      return false;
    }
    return true;
  }
  static bool loadExifDataFromImage(const std::string &pathToImage, glm::uvec2 &fullPanoSize, glm::uvec2 &croppedImageSize, glm::uvec2 &croppedImagePos) {
    static const std::string pathToExiv2 = "C:\\bin\\exiv2\\exiv2.exe";
    static const std::string exiv2Params = "-pa print";
    static const std::string tempFile = "temp.txt";

    // Extract XMP data from image, write to temp file
    std::string command = pathToExiv2 + " " + exiv2Params + " " + pathToImage + " > " + tempFile;
    system(command.c_str());
    std::string exifData = oria::readFile(tempFile);
    if (parseExifData(exifData, fullPanoSize, croppedImageSize, croppedImagePos)) {
      remove(tempFile.c_str());
      return true;
    }

    std::cout << "output has been left in " + tempFile << std::endl;
    return false;
  }
};

RUN_OVR_APP(PhotoSphereExample);
