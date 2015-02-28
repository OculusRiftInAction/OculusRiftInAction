/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ************************************************************************************/

#pragma once


/**
* Conversion between GLM and Oculus math types
*/
namespace ovr {

  inline mat4 toGlm(const ovrMatrix4f & om) {
    return glm::transpose(glm::make_mat4(&om.M[0][0]));
  }

  inline mat4 toGlm(const ovrFovPort & fovport, float nearPlane = 0.01f, float farPlane = 10000.0f) {
    return toGlm(ovrMatrix4f_Projection(fovport, nearPlane, farPlane, true));
  }

  inline vec3 toGlm(const ovrVector3f & ov) {
    return glm::make_vec3(&ov.x);
  }

  inline vec2 toGlm(const ovrVector2f & ov) {
    return glm::make_vec2(&ov.x);
  }

  inline uvec2 toGlm(const ovrSizei & ov) {
    return uvec2(ov.w, ov.h);
  }

  inline quat toGlm(const ovrQuatf & oq) {
    return glm::make_quat(&oq.x);
  }

  inline mat4 toGlm(const ovrPosef & op) {
    mat4 orientation = glm::mat4_cast(toGlm(op.Orientation));
    mat4 translation = glm::translate(mat4(), ovr::toGlm(op.Position));
    return translation * orientation;
  }

  inline ovrMatrix4f fromGlm(const mat4 & m) {
    ovrMatrix4f result;
    mat4 transposed(glm::transpose(m));
    memcpy(result.M, &(transposed[0][0]), sizeof(float) * 16);
    return result;
  }

  inline ovrVector3f fromGlm(const vec3 & v) {
    ovrVector3f result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    return result;
  }

  inline ovrVector2f fromGlm(const vec2 & v) {
    ovrVector2f result;
    result.x = v.x;
    result.y = v.y;
    return result;
  }

  inline ovrSizei fromGlm(const uvec2 & v) {
    ovrSizei result;
    result.w = v.x;
    result.h = v.y;
    return result;
  }

  inline ovrQuatf fromGlm(const quat & q) {
    ovrQuatf result;
    result.x = q.x;
    result.y = q.y;
    result.z = q.z;
    result.w = q.w;
    return result;
  }

  GLFWwindow * createRiftRenderingWindow(ovrHmd hmd, glm::uvec2 & outSize, glm::ivec2 & outPosition);
}

namespace oria {

  bool clearHSW(ovrHmd hmd);

}

// Convenience method for looping over each eye with a lambda
template <typename Function>
void for_each_eye(Function function) {
  for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
    eye < ovrEyeType::ovrEye_Count;
    eye = static_cast<ovrEyeType>(eye + 1)) {
    function(eye);
  }
}

// Combine some macros together to create a single macro
// to launch a class containing a run method
#define RUN_OVR_APP(AppClass) \
MAIN_DECL { \
  if (!ovr_Initialize()) { \
      SAY_ERR("Failed to initialize the Oculus SDK"); \
      return -1; \
  } \
  int result = -1; \
  try { \
    result = AppClass().run(); \
  } catch (std::exception & error) { \
    SAY_ERR(error.what()); \
  } catch (std::string & error) { \
    SAY_ERR(error.c_str()); \
  } \
  ovr_Shutdown(); \
  return result; \
}

