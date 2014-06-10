#include "Common.h"

class Example05 : public RiftApp {
public:

  Example05() {
    sensorFusion.SetGravityEnabled(true);
    sensorFusion.SetPredictionEnabled(true);
    CameraControl::instance().enableHydra(true);

    OVR::Util::Render::StereoConfig ovrStereoConfig;
    ovrStereoConfig.SetHMDInfo(ovrHmdInfo);
    gl::Stacks::projection().top() =
      glm::perspective(ovrStereoConfig.GetYFOVRadians(),
      glm::aspect(eyeSize), 0.01f, 1e12f);

  }

  void initGl() {
    RiftApp::initGl();
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_ERROR;

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    GL_CHECK_ERROR;
  }

  virtual void renderScene() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();

    GlUtils::renderSkybox(Resource::IMAGES_SKY_SPACE_XNEG_PNG);
//    GlUtils::renderFloorGrid(player);

    static gl::GeometryPtr sphere; if (!sphere) {
      Mesh mesh = GlUtils::getMesh(Resource::MESHES_SPHERE_CTM);
      mesh.fillNormals(true);
      sphere = mesh.getGeometry();
    }
    static gl::Texture2dPtr color; 
    static gl::Texture2dPtr night;
    static gl::Texture2dPtr bump;
    static gl::Texture2dPtr specular;
    static gl::Texture2dPtr clouds;

    if (!color) {
      GlUtils::getImageAsTexture(color, Resource::IMAGES_TEXTURES_EARTH_COLORMAP_PNG);
      SAY("Loaded color");
      GlUtils::getImageAsTexture(night, Resource::IMAGES_TEXTURES_EARTH_NIGHTLIGHTS_PNG);
      SAY("Loaded night");
      GlUtils::getImageAsTexture(bump, Resource::IMAGES_TEXTURES_EARTH_BUMP_PNG);
      SAY("Loaded bump");
      GlUtils::getImageAsTexture(specular, Resource::IMAGES_TEXTURES_EARTH_SPECMASK_PNG);
      SAY("Loaded specmask");
      GlUtils::getImageAsTexture(clouds, Resource::IMAGES_TEXTURES_EARTH_CLOUDS_PNG);
      SAY("Loaded clouds");
    }

    gl::Stacks::with_push(mv, [&]{
      gl::ProgramPtr program = GlUtils::getProgram(
        Resource::SHADERS_PLANET_VS,
        Resource::SHADERS_PLANET_FS);
      program->use();

      program->setUniform("Time", Platform::elapsedMillis());

      glm::mat3 normalMatrix(mv.top());
      program->setUniform("NormalMatrix", normalMatrix);

      glm::vec3 sunVector = normalMatrix * glm::normalize(glm::vec3(1, 1, 0));
      program->setUniform("SunVector", sunVector);

      glm::vec3 halfVector = normalMatrix * glm::normalize(sunVector + glm::normalize(glm::vec3(player[3])));
      program->setUniform("HalfVector", halfVector);

      program->setUniform("Night", 1);
      glActiveTexture(GL_TEXTURE1);
      night->bind();
      program->setUniform("Bump", 2);
      glActiveTexture(GL_TEXTURE2);
      bump->bind();
      program->setUniform("Specular", 3);
      glActiveTexture(GL_TEXTURE3);
      specular->bind();
      program->setUniform("Clouds", 4);
      glActiveTexture(GL_TEXTURE4);
      clouds->bind();
      program->setUniform("Color", 0);
      glActiveTexture(GL_TEXTURE0);
      color->bind();
      static float EARTH_RADIUS = 6378100;

      mv.translate(glm::vec3(0, 0, -EARTH_RADIUS * 2.0f));
      mv.scale(EARTH_RADIUS);
      color->bind();
      GlUtils::renderGeometry(sphere, program);
      gl::Texture2d::unbind();
    });

//gl::ProgramPtr program = GlUtils::getProgram(
//  Resource::SHADERS_COLORED_VS, Resource::SHADERS_COLORED_FS);
//program->use();
//    gl::Stacks::projection().apply(program);
//    gl::GeometryPtr colorCube = GlUtils::getColorCubeGeometry();
//    colorCube->bindVertexArray();
//    glm::vec2 p2 = quantize(position, 1.0);
//    for (int x = -10; x <= 10; ++x) {
//      for (int y = -10; y <= 10; ++y) {
//        glm::vec2 offset(x, y);
//        offset += p2;
////        float n = 0;
//        float n = glm::perlin(offset / 10.0f);
//        //mv.push().translate(glm::vec3(offset.x, n, offset.y)).scale(abs(n) / 4.0f).apply(program).pop();
//        //colorCube->draw();
//      }
//    }
//    gl::VertexArray::unbind();
//    gl::Program::clear();
//    gl::Stacks::projection().push().identity().top() =
//        eyeArgs.projectionOffset *
//          glm::ortho(-1.0f, 1.0f,
//            -glm::aspect(windowSize),
//            glm::aspect(windowSize));
//    glm::quat playerOrientation(player);
//    gl::Stacks::modelview().push().identity().translate(
//        glm::vec3(0.3f, -0.2f, 0.0f)).scale(0.2f).
//        rotate(glm::inverse(playerOrientation)).
//        rotate(riftOrientation);
//
//    GlUtils::renderArtificialHorizon(0.5);
//
//    gl::Stacks::modelview().pop();
//    gl::Stacks::projection().pop();

//    lineWidth(2.0);
//    mv.push().scale(10).translate(glm::mod(tr, 20.0f));
//    GlUtils::draw3dGrid();
//    mv.pop();

//    9 -> 0
//    -9 -> 0
//    11 -> 20



//    lineWidth(3.0);
//    mv.push().translate(glm::mod(tr, 200.0f)).scale(100);
//    GlUtils::draw3dGrid();
//    mv.pop();

//    mv.scale(10);
//    GlUtils::drawColorCube();

    // Set up camera position
//    mv.push();
//    {
//      mv.translate(vec3(0, 0, -0.5));
////      GlUtils::renderGeometry(treeGeometry,
////          Resource::SHADERS_LITCOLORED_VS,
////          Resource::SHADERS_LITCOLORED_FS);
//    }
//    mv.pop();

//    mv.push();
//    {
////    mv.translate(curPos / 1000.0f);
////    mv.rotate(curOrient);
//      mv.translate(GlUtils::Y_AXIS * -0.3f);
//      mv.scale(glm::vec3(4));
////    GlUtils::renderBunny();
//      mv.push();
//      {
//        mv.identity();
//        glm::vec2 cursor;
////        GlUtils::renderString("Hello", cursor);
//      }
//      mv.pop();
//    }
//    mv.pop();
//    GlUtils::draw3dVector(mag, Colors::cyan);
  }

};

RUN_OVR_APP(Example05);
