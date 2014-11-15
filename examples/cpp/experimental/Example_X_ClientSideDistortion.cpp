#include "Common.h"

struct EyeArg {
  FramebufferWrapper            frameBuffer;
  glm::vec2                     scale;
  glm::vec2                     offset;

  ovrDistortionMesh             mesh;
  oglplus::Buffer               meshBuffer;
  oglplus::Buffer               meshIndexBuffer;
  oglplus::VertexArray          meshVao;
  glm::mat4                     projection;
};

typedef std::shared_ptr<EyeArg> EyeArgPtr;


class ClientSideDistortionExample : public RiftGlfwApp {
  EyeArgPtr   eyeArgs[2];
  ovrVector3f hmdToEyeOffsets[2];
  glm::mat4   player;
  float       ipd = OVR_DEFAULT_IPD;
  float       eyeHeight = OVR_DEFAULT_EYE_HEIGHT;
  ProgramPtr  distortionProgram;

public:
  ClientSideDistortionExample() {
    ovrHmd_ConfigureTracking(hmd, 
      ovrTrackingCap_Orientation |
      ovrTrackingCap_Position, 0);
    ovrHmd_ResetFrameTiming(hmd, 0);
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, ipd * 4),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),  // Where the camera is looking
      Vectors::Y_AXIS));           // Camera up axis
  }

  void initGl() {
    RiftGlfwApp::initGl();
    using namespace oglplus;
    distortionProgram = oria::loadProgram(
      Resource::SHADERS_DISTORTION_VS,
      Resource::SHADERS_DISTORTION_FS
    );

    for_each_eye([&](ovrEyeType eye){
      eyeArgs[eye] = EyeArgPtr(new EyeArg());
      EyeArg & eyeArg = *eyeArgs[eye];
      ovrFovPort fov = hmd->DefaultEyeFov[eye];
      ovrEyeRenderDesc renderDesc = ovrHmd_GetRenderDesc(hmd, eye, fov);

      // Set up the per-eye projection matrix
      eyeArg.projection = ovr::toGlm(
        ovrMatrix4f_Projection(fov, 0.01, 100000, true));
      hmdToEyeOffsets[eye] = renderDesc.HmdToEyeViewOffset;
      ovrRecti texRect;
      texRect.Size = ovrHmd_GetFovTextureSize(hmd, eye,
        hmd->DefaultEyeFov[eye], 1.0f);
      texRect.Pos.x = texRect.Pos.y = 0;

      eyeArg.frameBuffer.init(ovr::toGlm(texRect.Size));

      ovrVector2f scaleAndOffset[2];
      ovrHmd_GetRenderScaleAndOffset(fov, texRect.Size,
        texRect, scaleAndOffset);
      eyeArg.scale = ovr::toGlm(scaleAndOffset[0]);
      eyeArg.offset = ovr::toGlm(scaleAndOffset[1]);

      ovrHmd_CreateDistortionMesh(hmd, eye, fov, 0, &eyeArg.mesh);

      eyeArg.meshVao.Bind();
      eyeArg.meshIndexBuffer.Bind(Buffer::Target::ElementArray);
      eyeArg.meshIndexBuffer.Data(Buffer::Target::ElementArray, eyeArg.mesh.IndexCount, eyeArg.mesh.pIndexData);
      eyeArg.meshBuffer.Bind(Buffer::Target::Array);
      eyeArg.meshBuffer.Data(Buffer::Target::Array, eyeArg.mesh.VertexCount, eyeArg.mesh.pVertexData);

      size_t stride = sizeof(ovrDistortionVertex);
      size_t offset = offsetof(ovrDistortionVertex, ScreenPosNDC);
      VertexArrayAttrib(oria::Layout::Attribute::Position)
        .Pointer(2, DataType::Float, false, stride, (void*)offset)
        .Enable();

      offset = offsetof(ovrDistortionVertex, TanEyeAnglesG);
      VertexArrayAttrib(oria::Layout::Attribute::TexCoord0)
        .Pointer(2, DataType::Float, false, stride, (void*)offset)
        .Enable();
      NoVertexArray().Bind();
    });

  }

  void update() {
    Stacks::modelview().top() = glm::inverse(player);
  }

  void draw() {
    using namespace oglplus;
    static int frameIndex = 0;
    ++frameIndex;
    Context::ClearColor(0, 0, 0, 1);
    Context::Disable(Capability::Blend);
    Context::Disable(Capability::CullFace);
    Context::Disable(Capability::DepthTest);
    Context::Clear().ColorBuffer();

    ovrPosef eyePoses[2];
    ovrHmd_GetEyePoses(hmd, frameIndex, hmdToEyeOffsets, eyePoses, nullptr);

    ovrFrameTiming timing = ovrHmd_BeginFrameTiming(hmd, frameIndex);

    for (int i = 0; i < 2; ++i) {
      const ovrEyeType eye = hmd->EyeRenderOrder[i];
      EyeArg & eyeArg = *eyeArgs[eye];
      // Set up the per-eye projection matrix
      Stacks::projection().top() = eyeArg.projection;
      
      eyeArg.frameBuffer.Bind();
      MatrixStack & mv = Stacks::modelview();
      Stacks::withPush([&]{
        // Set up the per-eye modelview matrix
        // Apply the head pose
        mv.preMultiply(glm::inverse(ovr::toGlm(eyePoses[eye])));
        renderScene();
      });
    }
    DefaultFramebuffer().Bind(oglplus::Framebuffer::Target::Draw);

    distortionProgram->Bind();
    bool showMesh = false;
    glViewport(0, 0, getSize().x, getSize().y);
//    float mix = (sin(ovr_GetTimeInSeconds() * TWO_PI / 10.0f) + 1.0f) / 2.0f;
    for_each_eye([&](ovrEyeType eye) {
      const EyeArg & eyeArg = *eyeArgs[eye];
      Uniform<vec2>(*distortionProgram, "EyeToSourceUVScale").Set(eyeArg.scale);
      Uniform<vec2>(*distortionProgram, "EyeToSourceUVOffset").Set(eyeArg.offset);
      Uniform<GLuint>(*distortionProgram, "RightEye").Set(ovrEye_Left == eye ? 0 : 1);
//      Uniform<GLfloat>(*distortionProgram, "DistortionWeight").Set(mix);
      eyeArg.frameBuffer.color->Bind(Texture::Target::_2D);
      eyeArg.meshVao.Bind();
      if (showMesh) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(3.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      }
      glDrawElements(GL_TRIANGLES, eyeArg.mesh.IndexCount,
        GL_UNSIGNED_SHORT, nullptr);
      if (showMesh) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      }
    });
    DefaultTexture().Bind(Texture::Target::_2D);;
    NoProgram().Bind();
    Context::Enable(Capability::Blend);
    Context::Enable(Capability::CullFace);
    Context::Enable(Capability::DepthTest);
    ovrHmd_EndFrameTiming(hmd);
  }

  virtual void renderScene() {
    using namespace oglplus;
    Context::Enable(Capability::DepthTest);
    Context::ClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    Context::Clear().ColorBuffer().DepthBuffer();
    oria::renderCubeScene(OVR_DEFAULT_IPD, OVR_DEFAULT_EYE_HEIGHT);
  }
};


RUN_OVR_APP(ClientSideDistortionExample);
