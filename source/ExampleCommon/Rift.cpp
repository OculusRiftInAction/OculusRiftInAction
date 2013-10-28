#include "Common.h"

namespace RiftExamples {

const float Rift::MONO_FOV = 65.0;
const int Rift::FRAMEBUFFER_OBJECT_SCALE = 3;
const float Rift::ZFAR = 100.f;
const float Rift::ZNEAR = 0.01f;


float Rift::generateLensOffset(const float screenSize, const float lensSeparationDistance) {
    const float eyeScreenWidth = screenSize / 2.0f;
    // The viewport goes from -1,1.  We want to get the offset
    // of the lens from the center of the viewport, so we only
    // want to look at the distance from 0, 1, so we divide in
    // half again
    const float halfEyeScreenWidth = eyeScreenWidth / 2.0f;

    // The distance from the center of the display panel (NOT
    // the center of the viewport) to the lens axis
    const float lensDistanceFromScreenCenter = lensSeparationDistance / 2.0f;

    // Now we we want to turn the measurement from
    // millimeters into the range 0, 1
    const float lensDistanceFromViewportEdge = lensDistanceFromScreenCenter / halfEyeScreenWidth;

    // Finally, we want the distnace from the center, not the
    // distance from the edge, so subtract the value from 1
    float lensCenterOffset = 1.0f - lensDistanceFromViewportEdge;

    return lensCenterOffset;
}

float Rift::generateLensOffset(const OVR::HMDInfo & hmdInfo) {
    return Rift::generateLensOffset(hmdInfo.HScreenSize, hmdInfo.LensSeparationDistance);
}

GL::TexturePtr Rift::generateDisplacementTexture(const OVR::HMDInfo & hmdInfo) {
    glm::ivec2 size(hmdInfo.HResolution / 2, hmdInfo.VResolution);
    GL::ProgramPtr program = Platform::getProgram(Resource::SHADERS_VERTEXTORIFT_VS, Resource::SHADERS_GENERATEDISPLACEMENTMAP_FS);
    GL::TexturePtr texture = GL::TexturePtr(new GL::Texture());
    texture->init();
    texture->bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    GL::Texture::unbind();
    // Create a framebuffer for the target map
    GL::FrameBuffer frameBuffer(texture);

    GL_CHECK_ERROR;
    frameBuffer.init(size.x, size.y);
    frameBuffer.activate();
    glViewport(0, 0, size.x, size.y);
    glClearColor(0, 1, 0, 1);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR;
    GL::Geometry quadGeometry(GlUtils::getQuadVertices(), GlUtils::getQuadIndices(), 2);
    GL_CHECK_ERROR;

    {
        program->use();
        program->setUniform1i("Mirror", 0);
        bindDistortionUniforms(program, hmdInfo);
        quadGeometry.bindVertexArray();
        program->checkConfigured();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (GLvoid*) 0);
        GL::VertexArray::unbind();
        GL::Program::clear();
    }
    frameBuffer.deactivate();
    GL_CHECK_ERROR;
    GL::TexturePtr result = frameBuffer.detachTexture();
    return result;
}


void Rift::bindDistortionUniforms(const GL::ProgramPtr & program, const OVR::HMDInfo & hmdInfo) {
    OVR::Util::Render::StereoConfig stereoConfig;
    stereoConfig.SetHMDInfo(hmdInfo);
    float lensOffset = Rift::generateLensOffset(hmdInfo);
    glm::ivec2 size(hmdInfo.HResolution / 2, hmdInfo.VResolution);
    glm::vec4 K(hmdInfo.DistortionK[0], hmdInfo.DistortionK[1], hmdInfo.DistortionK[2], hmdInfo.DistortionK[3]);
    program->setUniform2f("Aspect", glm::vec2(1.0, ((float)hmdInfo.HResolution / 2.0f) / (float)hmdInfo.VResolution));
    program->setUniform2f("LensCenter", glm::vec2(Rift::generateLensOffset(hmdInfo), 0.0f));
    program->setUniform2f("DistortionScale", glm::vec2(1.0f / stereoConfig.GetDistortionScale()));
    program->setUniform4f("K", K);
    GL_CHECK_ERROR;
}

void Rift::loadDefaultHmdValues(OVR::HMDInfo & hmdInfo) {
    hmdInfo.HResolution = 1280;
    hmdInfo.VResolution = 800;
    hmdInfo.HScreenSize = 0.14976f;
    hmdInfo.VScreenSize = 0.09360f;
    hmdInfo.VScreenCenter = 0.04680f;
    hmdInfo.EyeToScreenDistance = 0.04100f;
    hmdInfo.LensSeparationDistance = 0.06350f;
    hmdInfo.InterpupillaryDistance = 0.06400f;
    hmdInfo.DistortionK[0] = 1;
    // hmdInfo.DistortionK[1] = 0.18007f;
    hmdInfo.DistortionK[1] = 0.22f;
    // hmdInfo.DistortionK[2] = 0.11502f;
    hmdInfo.DistortionK[2] = 0.24f;
    hmdInfo.DistortionK[3] = 0;
    hmdInfo.ChromaAbCorrection[0] =  0.99600f;
    hmdInfo.ChromaAbCorrection[1] = -0.00400f;
    hmdInfo.ChromaAbCorrection[2] =  1.01400f;
    hmdInfo.ChromaAbCorrection[3] =  0;
}

glm::vec3 Rift::getEulerAngles(OVR::SensorFusion & sensorFusion) {
    glm::vec3 eulerAngles;
    sensorFusion.GetOrientation().GetEulerAngles<
            OVR::Axis_X, OVR::Axis_Y, OVR::Axis_Z,
            OVR::Rotate_CW, OVR::Handed_R
        >(&eulerAngles.x, &eulerAngles.y, &eulerAngles.z);
    return eulerAngles;
}

glm::quat Rift::getQuaternion(OVR::SensorFusion & sensorFusion) {
    return glm::quat(getEulerAngles(sensorFusion));
}

const int FRAMEBUFFER_OBJECT_SCALE = 3;

RiftApp::RiftApp()
        /// : renderMode(OVR::Util::Render::Stereo_None) {
        : renderMode(OVR::Util::Render::Stereo_LeftRight_Multipass) {
    ///////////////////////////////////////////////////////////////////////////
    // Initialize Oculus VR SDK and hardware
    ovrManager = *OVR::DeviceManager::Create();
    bool attached = false;
    if (ovrManager) {
        ovrSensor =
                *ovrManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();

        if (ovrSensor) {
            sensorFusion.AttachToSensor(ovrSensor);
            attached = sensorFusion.IsAttachedToSensor();
        }

        // The SensorFusion object retains a reference to the sensor object, so we don't
        // need to maintain a link to it
        attached = sensorFusion.IsAttachedToSensor();

        OVR::Ptr<OVR::HMDDevice> ovrHmd = *ovrManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
        if (ovrHmd) {
            ovrHmd->GetDeviceInfo(&hmdInfo);
        } else {
            Rift::loadDefaultHmdValues(hmdInfo);
        }
        Rift::loadDefaultHmdValues(hmdInfo);
        // The HMDInfo structure contains everything we need for now, so no
        // need to keep the device handle around
        ovrHmd.Clear();
    }
    attached = sensorFusion.IsAttachedToSensor();

    OVR::Util::Render::StereoConfig stereoConfig;
    stereoConfig.SetHMDInfo(hmdInfo);
    eyeWidth = hmdInfo.HResolution / 2;
    eyeHeight = hmdInfo.VResolution;

    // Compute the offsets for the modelview and projection matrices
    renderArgs[0].fov = renderArgs[1].fov = stereoConfig.GetYFOVDegrees();
    renderArgs[0].aspect = renderArgs[1].aspect = (float) eyeWidth / (float)eyeHeight;

    renderArgs[0].projectionOffset = glm::vec3(stereoConfig.GetProjectionCenterOffset() / 2.0f, 0, 0);
    renderArgs[0].modelviewOffset = glm::vec3(stereoConfig.GetIPD() / 2.0f, 0, 0);
    renderArgs[0].eye = OVR::Util::Render::StereoEye_Left;

    renderArgs[1].projectionOffset = -1.0f * renderArgs[0].projectionOffset;
    renderArgs[1].modelviewOffset = -1.0f * renderArgs[0].modelviewOffset;
    renderArgs[1].eye = OVR::Util::Render::StereoEye_Right;

    renderArgs[2].fov = Rift::MONO_FOV;
    renderArgs[2].aspect = (float) hmdInfo.HResolution / (float)hmdInfo.VResolution;
    renderArgs[2].eye = OVR::Util::Render::StereoEye_Center;

    // We don't want our Rift apps to have window decoration
    glfwWindowHint(GLFW_DECORATED, 0);
    ///////////////////////////////////////////////////////////////////////////
    // Create OpenGL window & context
    createWindow(hmdInfo.HResolution, hmdInfo.VResolution, hmdInfo.DesktopX, hmdInfo.DesktopY);
    GL_CHECK_ERROR;
}

RiftApp::~RiftApp() {
    sensorFusion.AttachToSensor(nullptr);
    ovrSensor.Clear();
    ovrManager.Clear();
}

void RiftApp::initGl() {
    ///////////////////////////////////////////////////////////////////////////
    // Initialize OpenGL settings and variables
    // Anti-alias lines (hopefully)
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glLineWidth(1.5f);
    GL_CHECK_ERROR;

    // Allocate the frameBuffer that will hold the scene, and then be
    // re-rendered to the screen with distortion
    frameBuffer.init(eyeWidth * FRAMEBUFFER_OBJECT_SCALE, eyeHeight * FRAMEBUFFER_OBJECT_SCALE);
    GL_CHECK_ERROR;

    // Create the buffers for the texture quad we will draw
    quadVertexBuffer = GlUtils::getQuadVertices();
    quadIndexBuffer = GlUtils::getQuadIndices();
    GL_CHECK_ERROR;

    // Create the rendering displacement map
    offsetTexture = Rift::generateDisplacementTexture(hmdInfo);
    GL_CHECK_ERROR;

    // Create the rendering shaders
    distortProgram = Platform::getProgram(Resource::SHADERS_VERTEXTOTEXTURE_VS, Resource::SHADERS_DISPLACEMENTMAPDISTORT_FS);
//    distortProgram = Platform::getProgram("VertexToRift.vs", "DirectDistort.fs");
//    distortProgram->use();
//    Rift::bindDistortionUniforms(distortProgram, hmdInfo);
//    GL::Program::clear();
    GL_CHECK_ERROR;
}


void RiftApp::onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
        return;
    }

    switch (key) {
    case GLFW_KEY_R:
        sensorFusion.Reset();
        break;

    case GLFW_KEY_P:
        if (renderMode == OVR::Util::Render::Stereo_None) {
            renderMode = OVR::Util::Render::Stereo_LeftRight_Multipass;
        } else {
            renderMode = OVR::Util::Render::Stereo_None;
        }
        break;
    }
}

void RiftApp::draw() {
    static RiftRenderResults results;
    if (renderMode == OVR::Util::Render::Stereo_None) {
        // If we're not working stereo, we're just going to render the
        // scene once, from a single position, directly to the back buffer
        glViewport(0, 0, hmdInfo.HResolution, hmdInfo.VResolution);
        renderScene(renderArgs[2], results);
        GL_CHECK_ERROR;
    } else {
        // If we get here, we're rendering in stereo, so we have to render our output twice
        static GL::Geometry quadGeometry(GlUtils::getQuadVertices(), GlUtils::getQuadIndices(), 2);

        // We have to explicitly clear the screen here.  the Clear command doesn't object the viewport
        // and the clear command inside renderScene will only target the active framebuffer object.
        glClearColor(0, 1, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GL_CHECK_ERROR;
        using namespace OVR::Util::Render;
        for (int i = 0; i < 2; ++i ) {
            // Compute the modelview and projection offsets for the rendered scene based on the eye and
            // whether or not we're doing side by side or rift rendering
            frameBuffer.activate();
            GL_CHECK_ERROR;
            renderScene(renderArgs[i], results);
            GL_CHECK_ERROR;
            frameBuffer.deactivate();
            GL_CHECK_ERROR;

            // Setup the viewport for the eye to which we're rendering
            glViewport(1 + (renderArgs[i].eye == StereoEye_Left ? 0 : eyeWidth), 1, eyeWidth - 2, eyeHeight - 2);

            distortProgram->use();
            distortProgram->setUniform1i("Mirror", renderArgs[i].eye == StereoEye_Left ? 0 : 1);

            distortProgram->setUniform1i("Scene", 0);
            glActiveTexture(GL_TEXTURE0);
            frameBuffer.getTexture()->bind();

            distortProgram->setUniform1i("OffsetMap", 1);
            glActiveTexture(GL_TEXTURE1);
            offsetTexture->bind();
            glActiveTexture(GL_TEXTURE0);
            {
                distortProgram->checkConfigured();
                quadGeometry.bindVertexArray();
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (GLvoid*) 0);
                GL::VertexArray::unbind();
            }
            GL::Program::clear();
        } // for
    } // if
}

}
