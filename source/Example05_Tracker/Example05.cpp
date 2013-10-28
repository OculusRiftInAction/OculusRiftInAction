#include "Common.h"
#include <list>
#include <algorithm>

namespace RiftExamples {

class Example05 : public GlfwApp {
protected:
    OVR::Ptr<OVR::DeviceManager> ovrManager;
    OVR::Ptr<OVR::SensorDevice> ovrSensor;
    OVR::SensorFusion sensorFusion;

    typedef std::list<glm::vec3> List;
    List accBuf;
    List rotBuf;
    List magBuf;
    static const int bufSize = 10;

public:
    Example05() :accBuf(bufSize), rotBuf(bufSize), magBuf(bufSize) {
        ovrManager = *OVR::DeviceManager::Create();
        if (!ovrManager) {
            FAIL("Unable to create OVR device manager");
        }
        ovrSensor = ovrManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
        if (!ovrSensor) {
            FAIL("Unable to detect Rift head tracker");
        }
        sensorFusion.AttachToSensor(ovrSensor);
        createWindow(1024, 768);

    }

    virtual ~Example05() {
        ovrSensor.Clear();
        ovrManager.Clear();
    }

    virtual void initGl() {
        GL::Stacks::projection().top() =
                glm::perspective(60.0f, 4.0f/3.0f, 0.001f, 1000.0f);
        GL::Stacks::modelview().top() =
                glm::lookAt(glm::vec3(1.5, 1.0, 1.5),
                        glm::vec3(0), GlUtils::Y_AXIS);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glm::value_ptr(GL::Stacks::projection().top()));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(GL::Stacks::modelview().top()));
    }

    virtual void update() {
        accBuf.push_back(Rift::fromOvr(sensorFusion.GetAcceleration()));
        while (accBuf.size() > bufSize) {
            accBuf.pop_front();
        }
        rotBuf.push_back(Rift::fromOvr(sensorFusion.GetAngularVelocity()));
        while (rotBuf.size() > bufSize) {
            rotBuf.pop_front();
        }
        magBuf.push_back(Rift::fromOvr(sensorFusion.GetMagnetometer()));
        while (magBuf.size() > bufSize) {
            magBuf.pop_front();
        }
    }

    static glm::vec3 average(const List & container) {
        glm::vec3 result;
        std::for_each(container.begin(), container.end(), [&](List::const_reference v) {
            result += v;
        });
        result /= container.size();
        return result;
    }

    virtual void draw() {
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::vec3 acc = average(accBuf);
        glm::vec3 mag = average(magBuf);
        glm::vec3 rot = average(rotBuf);

        acc /= 9.8f;
        float accLen = glm::length(acc);
        if (accLen > 1.0f) {
            acc /= accLen;
        }
        float rotLen = glm::length(rot);
        if (rotLen > 1.0f) {
            rot /= rotLen;
        }
        float magLen = glm::length(mag);
        if (magLen > 1.0f) {
            mag /= magLen;
        }

        glLineWidth(1.0);
        GlUtils::draw3dGrid();

        glBegin(GL_LINES);
            GL::color(glm::vec3(0.6f));

            GL::vertex(glm::vec3(0));
            GL::vertex(glm::vec3(acc.x, 0, acc.z));
            GL::vertex(glm::vec3(acc.x, 0, acc.z));
            GL::vertex(acc);

            GL::vertex(glm::vec3(0));
            GL::vertex(glm::vec3(rot.x, 0, rot.z));
            GL::vertex(glm::vec3(rot.x, 0, rot.z));
            GL::vertex(rot);

            GL::vertex(glm::vec3(0));
            GL::vertex(glm::vec3(mag.x, 0, mag.z));
            GL::vertex(glm::vec3(mag.x, 0, mag.z));
            GL::vertex(mag);
        glEnd();

        glLineWidth(2.0 + rotLen);
        glBegin(GL_LINES);
            GL::color(glm::vec3(1, 1, 0));
            GL::vertex(glm::vec3(0));
            GL::vertex(rot);
        glEnd();

        glLineWidth(2.0 + magLen);
        glBegin(GL_LINES);
            GL::color(glm::vec3(0, 1, 1));
            GL::vertex(glm::vec3(0));
            GL::vertex(mag);
        glEnd();

        glLineWidth(2.0 + accLen);
        glBegin(GL_LINES);
            GL::color(glm::vec3(1, 1, 1));
            GL::vertex(glm::vec3(0));
            GL::vertex(acc);
        glEnd();
        GL_CHECK_ERROR
    }
};

}

RUN_OVR_APP(RiftExamples::Example05);
