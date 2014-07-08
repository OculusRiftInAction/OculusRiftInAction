#include "Common.h"
#include <jansson.h>

struct EyeArg {
  ovrDistortionMesh             mesh;
  glm::vec2                     scale;
  glm::vec2                     offset;
  glm::uvec2                    textureSize;
  glm::mat4                     projection;
};

json_t * toJson(const float * fp, size_t size) {
  json_t * result = json_array();
  for (int i = 0; i < size; ++i) {
    json_array_append(result, json_real(fp[i]));
  }
  return result;
}

json_t * toJson(const unsigned short * ip, size_t size) {
  json_t * result = json_array();
  for (int i = 0; i < size; ++i) {
    json_array_append(result, json_integer(ip[i]));
  }
  return result;
}

json_t * toJson(const unsigned int * ip, size_t size) {
  json_t * result = json_array();
  for (int i = 0; i < size; ++i) {
    json_array_append(result, json_integer(ip[i]));
  }
  return result;
}

json_t * toJson(const glm::mat4 & m) {
  return toJson(glm::value_ptr(m), 16);
}

json_t * toJson(const glm::vec2 & v) {
  return toJson(glm::value_ptr(v), 2);
}

json_t * toJson(const glm::uvec2 & v) {
  return toJson(glm::value_ptr(v), 2);
}

json_t * toJson(const glm::vec3 & v) {
  return toJson(glm::value_ptr(v), 3);
}

void appendVertex(json_t * array, const ovrDistortionVertex & vertex) {
  json_array_append(array, json_real(vertex.Pos.x));
  json_array_append(array, json_real(vertex.Pos.y));
  json_array_append(array, json_real(vertex.TimeWarpFactor));
  json_array_append(array, json_real(vertex.VignetteFactor));
  json_array_append(array, json_real(vertex.TexR.x));
  json_array_append(array, json_real(vertex.TexR.y));
  json_array_append(array, json_real(vertex.TexG.x));
  json_array_append(array, json_real(vertex.TexG.y));
  json_array_append(array, json_real(vertex.TexB.x));
  json_array_append(array, json_real(vertex.TexB.y));
}

json_t * toJson(const ovrDistortionVertex & vertex) {
  json_t * result = json_array();
  appendVertex(result, vertex);
  return result;
}


json_t * toJson(const ovrDistortionVertex * vp, size_t size) {
  json_t * result = json_array();
  for (int i = 0; i < size; ++i) {
    appendVertex(result, vp[i]);
//    json_array_append(result, toJson(vp[i]));
  }
  return result;
}


json_t * toJson(const ovrDistortionMesh & mesh) {
  json_t * result = json_object();
  json_object_set(result, "indices", toJson(mesh.pIndexData, mesh.IndexCount));
  json_object_set(result, "vertices", toJson(mesh.pVertexData, mesh.VertexCount));
  return result;
}

class GenerateDistortionMesh : public RiftManagerApp {
  EyeArg      eyeArgs[2];
  glm::mat4   player;
  gl::ProgramPtr distortionProgram;

public:
  GenerateDistortionMesh() :
    RiftManagerApp(ovrHmd_DK1) { }

  int run() {
    for_each_eye([&](ovrEyeType eye){
      EyeArg & eyeArg = eyeArgs[eye];
      ovrFovPort fov = hmdDesc.DefaultEyeFov[eye];

      // Set up the per-eye projection matrix
      ovrMatrix4f projection =
          ovrMatrix4f_Projection(fov, 0.01, 100000, true);
      eyeArg.projection = Rift::fromOvr(projection);
//      ovrEyeRenderDesc renderDesc = ovrHmd_GetRenderDesc(hmd, eye, fov);
//      eyeArg.viewOffset = Rift::fromOvr(renderDesc.ViewAdjust);

      ovrRecti texRect;
      texRect.Size = ovrHmd_GetFovTextureSize(hmd, eye,
          hmdDesc.DefaultEyeFov[eye], 1.0f);
      texRect.Pos.x = texRect.Pos.y = 0;
      eyeArg.textureSize = Rift::fromOvr(texRect.Size);

      ovrVector2f scaleAndOffset[2];
      ovrHmd_GetRenderScaleAndOffset(fov, texRect.Size,
        texRect, scaleAndOffset);
      eyeArg.scale = Rift::fromOvr(scaleAndOffset[0]);
      eyeArg.offset = Rift::fromOvr(scaleAndOffset[1]);

      ovrHmd_CreateDistortionMesh(hmd, eye, fov, 0, &eyeArg.mesh);
    });

    json_t * root = json_object();
    for_each_eye([&](ovrEyeType eye){
      json_t * jsonEye = json_object();
      if (eye == ovrEye_Left) {
        json_object_set(root, "left", jsonEye);
      } else {
        json_object_set(root, "right", jsonEye);
      }
      EyeArg & eyeArg = eyeArgs[eye];
      json_object_set(jsonEye, "scale", toJson(eyeArg.scale));
      json_object_set(jsonEye, "offset", toJson(eyeArg.offset));
      json_object_set(jsonEye, "projection", toJson(eyeArg.projection));
//      json_object_set(jsonEye, "viewOffset", toJson(eyeArg.viewOffset));
      json_object_set(jsonEye, "textureSize", toJson(eyeArg.textureSize));
      json_object_set(jsonEye, "mesh", toJson(eyeArg.mesh));
    });
    json_dump_file(root, "mesh.json", JSON_SORT_KEYS | JSON_INDENT(2));
    return 0;
  }
};


RUN_OVR_APP(GenerateDistortionMesh);
