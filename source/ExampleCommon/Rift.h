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
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "GlfwApp.h"
#include "GlTexture.h"
#include "GlBuffers.h"
#include "GlShaders.h"
#include "GlUtils.h"
#include "GlFrameBuffer.h"
#include <OVR.h>
#undef new

namespace RiftExamples {

class Rift {
public:
    static float generateLensOffset(float hScreenSize, float lsd);
    static float generateLensOffset(const OVR::HMDInfo &);
    static GL::TexturePtr generateDisplacementTexture(const OVR::HMDInfo & hmdInfo);
    static void bindDistortionUniforms(const GL::ProgramPtr & program, const OVR::HMDInfo & hmdInfo);
    static void loadDefaultHmdValues(OVR::HMDInfo & hmdInfo);
    static glm::quat getQuaternion(OVR::SensorFusion & sensorFusion);
    static glm::vec3 getEulerAngles(OVR::SensorFusion & sensorFusion);

    static glm::vec3 fromOvr(const OVR::Vector3f & in) {
        return glm::vec3(in.x, in.y, in.z);
    }

    static glm::vec3 getEulerAngles(OVR::Quatf & in) {
        glm::vec3 eulerAngles;
        in.GetEulerAngles<
                OVR::Axis_X, OVR::Axis_Y, OVR::Axis_Z,
                OVR::Rotate_CW, OVR::Handed_R
            >(&eulerAngles.x, &eulerAngles.y, &eulerAngles.z);
        return eulerAngles;
    }

    static glm::quat fromOvr(OVR::Quatf & in) {
        return glm::quat(getEulerAngles(in));
    }


    static const float MONO_FOV;
    static const int FRAMEBUFFER_OBJECT_SCALE;
    static const float ZFAR;
    static const float ZNEAR;

};

struct RiftRenderArgs {
    glm::vec3 projectionOffset;
    glm::vec3 modelviewOffset;
    float aspect;
    float fov;
    OVR::Util::Render::StereoEye eye;

    glm::mat4 buildProjection(bool offset = true) const {
        glm::mat4 projection = glm::perspective(fov, aspect, Rift::ZNEAR, Rift::ZFAR);
        if (offset) {
            projection = glm::translate(glm::mat4(), projectionOffset) *
                    projection;
        }
        return projection;
    }
};

struct RiftRenderResults {
    float aspect;
    float fov;
    bool projectionOffsetApplied;
};

class RiftApp : public GlfwApp {
protected:
    OVR::Ptr<OVR::DeviceManager> ovrManager;
    OVR::Util::Render::StereoMode renderMode;
    OVR::SensorFusion sensorFusion;
    OVR::Ptr<OVR::SensorDevice> ovrSensor;

    // Provides the resolution and location of the Rift
    OVR::HMDInfo hmdInfo;

    // Calculated width and height of the per-eye rendering area used
    int eyeWidth, eyeHeight;

    GL::TexturePtr offsetTexture;
    GL::VertexBufferPtr quadVertexBuffer;
    GL::IndexBufferPtr quadIndexBuffer;
    GL::FrameBuffer frameBuffer;
    GL::ProgramPtr distortProgram;

    // Compute the modelview and projection matrices for the rendered scene based on the eye and
    // whether or not we're doing side by side or rift rendering
    RiftRenderArgs renderArgs[3];

public:

    RiftApp();
    virtual ~RiftApp();
    virtual void initGl();
    virtual void onKey(int key, int scancode, int action, int mods);
    virtual void draw();
    // This method should be overridden in derived classes in order to render
    // the scene.  The idea FoV
    virtual void renderScene(
            const RiftRenderArgs & renderArgs,
            RiftRenderResults & renderResults
        ) = 0;
};

}

// Combine some macros together to create a single macro
// to run an example app
#define RUN_OVR_APP(AppClass) \
    MAIN_DECL { \
        OVR::System::Init(); \
        try { \
            return AppClass().run(); \
        } catch (const std::string & error) { \
            SAY(error.c_str()); \
        } \
        OVR::System::Destroy(); \
        return -1; \
    }

