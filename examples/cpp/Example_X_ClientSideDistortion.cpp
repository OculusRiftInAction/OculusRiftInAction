#include "Common.h"

struct EyeArg {
  gl::FrameBufferWrapper        frameBuffer;
  glm::vec2                     scale;
  glm::vec2                     offset;

  ovrDistortionMesh             mesh;
  gl::VertexBufferPtr           meshVbo;
  gl::IndexBufferPtr            meshIbo;
  gl::VertexArrayPtr            meshVao;

  glm::mat4                     projection;
  glm::mat4                     viewOffset;
};

class ClientSideDistortionExample : public RiftGlfwApp {
  EyeArg      eyeArgs[2];
  glm::mat4   player;
  float       ipd = OVR_DEFAULT_IPD;
  float       eyeHeight = OVR_DEFAULT_EYE_HEIGHT;
  gl::ProgramPtr distortionProgram;

public:
  ClientSideDistortionExample() {
    ovrHmd_StartSensor(hmd, 0, 0);
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, ipd * 4),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),  // Where the camera is looking
      GlUtils::Y_AXIS));           // Camera up axis
  }

  void initGl() {
    RiftGlfwApp::initGl();
    for_each_eye([&](ovrEyeType eye){
      EyeArg & eyeArg = eyeArgs[eye];
      ovrFovPort fov = hmdDesc.DefaultEyeFov[eye];
      ovrEyeRenderDesc renderDesc = ovrHmd_GetRenderDesc(hmd, eye, fov);

      // Set up the per-eye projection matrix
      eyeArg.projection = Rift::fromOvr(
        ovrMatrix4f_Projection(fov, 0.01, 100000, true));
      eyeArg.viewOffset = glm::translate(glm::mat4(), Rift::fromOvr(renderDesc.ViewAdjust));
      ovrRecti texRect;
      texRect.Size = ovrHmd_GetFovTextureSize(hmd, eye,
        hmdDesc.DefaultEyeFov[eye], 1.0f);
      texRect.Pos.x = texRect.Pos.y = 0;

      eyeArg.frameBuffer.init(Rift::fromOvr(texRect.Size));

      ovrVector2f scaleAndOffset[2];
      ovrHmd_GetRenderScaleAndOffset(fov, texRect.Size,
        texRect, scaleAndOffset);
      eyeArg.scale = Rift::fromOvr(scaleAndOffset[0]);
      eyeArg.offset = Rift::fromOvr(scaleAndOffset[1]);

      ovrHmd_CreateDistortionMesh(hmd, eye, fov, 0, &eyeArg.mesh);

      eyeArg.meshVao = gl::VertexArrayPtr(new gl::VertexArray());
      eyeArg.meshVao->bind();

      eyeArg.meshIbo = gl::IndexBufferPtr(new gl::IndexBuffer());
      eyeArg.meshIbo->bind();
      size_t bufferSize = eyeArg.mesh.IndexCount * sizeof(unsigned short);
      eyeArg.meshIbo->load(bufferSize, eyeArg.mesh.pIndexData);

      eyeArg.meshVbo = gl::VertexBufferPtr(new gl::VertexBuffer());
      eyeArg.meshVbo->bind();
      bufferSize = eyeArg.mesh.VertexCount * sizeof(ovrDistortionVertex);
      eyeArg.meshVbo->load(bufferSize, eyeArg.mesh.pVertexData);

      size_t stride = sizeof(ovrDistortionVertex);
      size_t offset = offsetof(ovrDistortionVertex, Pos);
      glEnableVertexAttribArray(gl::Attribute::Position);
      glVertexAttribPointer(gl::Attribute::Position, 2, GL_FLOAT, GL_FALSE,
        stride, (void*)offset);

      offset = offsetof(ovrDistortionVertex, TexG);
      glEnableVertexAttribArray(gl::Attribute::TexCoord0);
      glVertexAttribPointer(gl::Attribute::TexCoord0, 2, GL_FLOAT, GL_FALSE,
        stride, (void*)offset);

      gl::VertexArray::unbind();
      gl::Program::clear();
    });

    distortionProgram = GlUtils::getProgram(
      Resource::SHADERS_DISTORTION_VS,
      Resource::SHADERS_DISTORTION_FS
    );
  }

  void update() {
    gl::Stacks::modelview().top() = glm::inverse(player);
  }

  void draw() {
    static int frameIndex = 0;
    ovrFrameTiming timing = ovrHmd_BeginFrameTiming(hmd, frameIndex++);
    for (int i = 0; i < 2; ++i) {
      const ovrEyeType eye = hmdDesc.EyeRenderOrder[i];
      const EyeArg & eyeArg = eyeArgs[eye];
      // Set up the per-eye projection matrix
      gl::Stacks::projection().top() = eyeArg.projection;

      eyeArg.frameBuffer.activate();
      gl::MatrixStack & mv = gl::Stacks::modelview();
      gl::Stacks::with_push([&]{
        ovrSensorState ss = ovrHmd_GetSensorState(hmd, timing.EyeScanoutSeconds[eye]);
        // Set up the per-eye modelview matrix
        // Apply the head pose
        mv.preMultiply(glm::inverse(Rift::fromOvr(ss.Predicted.Pose)));
        // Apply the per-eye offset
        mv.preMultiply(eyeArg.viewOffset);
        renderScene();
      });
      eyeArg.frameBuffer.deactivate();
    }

    glClearColor(0, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    distortionProgram->use();

    glViewport(0, 0, windowSize.x, windowSize.y);
    for_each_eye([&](ovrEyeType eye) {
      const EyeArg & eyeArg = eyeArgs[eye];
      distortionProgram->setUniform(0, eyeArg.scale);
      distortionProgram->setUniform(1, eyeArg.offset);
      eyeArg.frameBuffer.color->bind();
      eyeArg.meshVao->bind();
      glDrawElements(GL_TRIANGLES, eyeArg.mesh.IndexCount,
        GL_UNSIGNED_SHORT, nullptr);
    });
    gl::Texture2d::unbind();
    gl::Program::clear();
    ovrHmd_EndFrameTiming(hmd);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
  }

  virtual void renderScene() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    GlUtils::renderFloor();
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::Stacks::with_push(mv, [&]{
      mv.translate(glm::vec3(0, eyeHeight, 0)).scale(ipd);
      GlUtils::drawColorCube(true);
    });
  }
};


RUN_OVR_APP(ClientSideDistortionExample);
