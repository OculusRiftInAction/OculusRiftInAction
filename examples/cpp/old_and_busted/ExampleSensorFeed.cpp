#include "Common.h"
#include <boost/circular_buffer.hpp>
#include <boost/thread/mutex.hpp>


class SensorFeed {
protected:
  gl::Texture2dPtr sensorTextures[2];
  float rate{ 10.0 };
  gl::GeometryPtr quadGeometry;
  const glm::uvec2 texSize{ 2048 };
  int xoffset{ 0 };

public:
  SensorFeed() {

  }

  void initGl() {
    quadGeometry = GlUtils::getQuadGeometry();
    for (int i = 0; i < 2; ++i) {
      gl::Texture2dPtr & texture = sensorTextures[i];
      texture = gl::Texture2dPtr(new gl::Texture2d());
      texture->bind();
      texture->image2d(texSize, nullptr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }
    gl::Texture2d::unbind();
  }

  void render() {
    static long start = Platform::elapsedMillis();
    static gl::ProgramPtr program = GlUtils::getProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    glDisable(GL_DEPTH_TEST);
    program->use();
    program->setUniform("Alpha", 0.5f);
    sensorTextures[0]->bind();
    gl::Stacks::with_push([&]{
      gl::Stacks::projection().top() = glm::mat4();
      gl::Stacks::modelview().top() = glm::mat4();
      GlUtils::renderGeometry(quadGeometry, program);
    });
    gl::Texture2d::unbind();
    glEnable(GL_DEPTH_TEST);
    xoffset++;
    xoffset %= texSize.x;
  }

  void addPoint(float f, const glm::vec3 & color = glm::vec3(1)) {
    // Update Texture
    // GLAPI void GLAPIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);

    sensorTextures[0]->bind();
    static unsigned char colorBytes[3];
    colorBytes[0] = color.r * 255;
    colorBytes[1] = color.g * 255;
    colorBytes[2] = color.b * 255;
    glm::uvec2 offset(xoffset, f * (texSize.x / 2) + texSize.x / 2);
    glTexSubImage2D(GL_TEXTURE_2D, 0, offset.x, offset.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, colorBytes);
    gl::Texture2d::unbind();
  }
};

class SensorFusionPredictionExample : public GlfwApp, public OVR::MessageHandler, public RiftManagerApp {
  typedef boost::circular_buffer<OVR::MessageBodyFrame> TrackerBuffer;
  
  OVR::Ptr<OVR::SensorDevice> ovrSensorDevice;
  OVR::SensorFusion ovrSensorFusion;
  boost::mutex mutex;
  TrackerBuffer buffer{ 1000 * 10 * 2 };
  SensorFeed feed;

public:

  SensorFusionPredictionExample() {
    ovrSensorDevice = *ovrManager->EnumerateDevices<OVR::SensorDevice>().
      CreateDevice();
    if (!ovrSensorDevice) {
      FAIL("Unable to locate Rift sensor device");
    }

    ovrSensorFusion.SetGravityEnabled(true);
    ovrSensorFusion.SetPredictionEnabled(true);
    ovrSensorFusion.SetYawCorrectionEnabled(true);
    ovrSensorFusion.AttachToSensor(ovrSensorDevice);
    ovrSensorFusion.SetDelegateMessageHandler(this);
    gl::Stacks::modelview().top() = glm::lookAt(
      glm::vec3(0, 0, 2),
      glm::vec3(0, 0, 0),
      glm::vec3(0, 1, 0));
    gl::Stacks::projection().top() = glm::perspective(
      PI / 3.0f, 1.33f, 0.1f, 100.0f);
  }

  virtual ~SensorFusionPredictionExample() {
    ovrSensorFusion.SetDelegateMessageHandler(nullptr);
  }


  void initGl() {
    GlfwApp::initGl();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    feed.initGl();
    GL_CHECK_ERROR;
  }

  virtual void OnMessage(const OVR::Message& rawmsg) { 
    mutex.lock();
    buffer.push_back(static_cast<const OVR::MessageBodyFrame&>(rawmsg));
    mutex.unlock();
  }

  // Determines if handler supports a specific message type. Can
  // be used to filter out entire message groups. The result
  // returned by this function shouldn't change after handler creation.
  virtual bool SupportsMessageType(OVR::MessageType type) const { 
    return OVR::MessageType::Message_BodyFrame == type; 
  }

  virtual void createRenderingTarget() {
    createSecondaryScreenWindow(glm::uvec2(800, 600));
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      GlfwApp::onKey(key, scancode, action, mods);
      return;
    }

    switch (key) {
    case GLFW_KEY_R:
      ovrSensorFusion.Reset();
      return;
    }
    GlfwApp::onKey(key, scancode, action, mods);
  }

  void update() {
    mutex.lock();
    TrackerBuffer trackerBufferCopy(buffer);
    buffer.clear();
    mutex.unlock();

    size_t size = trackerBufferCopy.size();
    glm::vec3 acc, rot;
    std::for_each(trackerBufferCopy.begin(), trackerBufferCopy.end(),
      [&](const OVR::MessageBodyFrame & msg) {
      acc += Rift::fromOvr(msg.Acceleration);
      rot += Rift::fromOvr(msg.RotationRate);
    });
    acc /= size;
    rot /= size;
    glm::vec3 v = glm::abs(rot);
    glm::vec3 l = glm::log(v);
    glm::vec3 o;
    for (size_t i = 0; i < 3; ++i) {
      if (v[i] < 1.0) {
        o[i] = rot[i];
      } else {
        if (rot[i] < 0) {
          o[i] = -l[i];
        }
        else {
          o[i] = l[i];
        }
      }
    }
    o /= 8;
    o = glm::clamp(o, glm::vec3(-0.95f), glm::vec3(0.95f));
    feed.addPoint(o.x, Colors::red);
    feed.addPoint(o.y, Colors::green);
    feed.addPoint(o.z, Colors::blue);
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    std::string message = Platform::format(
        "Prediction Delta: %0.2f ms\n",
        ovrSensorFusion.GetPredictionDelta() * 1000.0f);
    renderStringAt(message, -0.9f, 0.9f);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    OVR::Matrix4f ovrMat = ovrSensorFusion.GetPredictedOrientation();
    glm::mat4 glmMat = glm::inverse(glm::make_mat4((float*)ovrMat.M));

    mv.with_push([&] {
      mv.transform(glmMat);
      GlUtils::renderRift();
    });
    feed.render();


    //glClear(GL_DEPTH_BUFFER_BIT);
    //gl::ProgramPtr program = GlUtils::getProgram(Resource::SHADERS_SIMPLE_VS, Resource::SHADERS_COLORED_FS);
    //program->use();
    //gl::Stacks::lights().apply(program);
    //gl::Stacks::projection().apply(program);
    //gl::Stacks::modelview().apply(program);
    //gyroVao->bind();
    //glDrawArrays(GL_LINE_STRIP, 0, vertexBufferEnd / STRIDE);
    //gl::VertexArray::unbind();
    //gl::Program::clear();
  }


  //int run() {
  //  while (true) {
  //    update();
  //    Platform::sleepMillis(15);
  //  }
  //  return 0;
  //}
};

RUN_OVR_APP(SensorFusionPredictionExample)

