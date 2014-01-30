#include "Common.h"

using namespace OVR;

class Tracker {
protected:
    Ptr<DeviceManager> ovrManager;

public:
    Tracker() {
        ovrManager = *DeviceManager::Create();
        if (!ovrManager) {
            FAIL("Unable to create OVR device manager");
        }
    }

    int run() {
        Ptr<SensorDevice> ovrSensor = *ovrManager->
                EnumerateDevices<SensorDevice>().CreateDevice();
        if (!ovrSensor) {
            SAY_ERR("Unable to detect Rift head tracker");
            return -1;
        }
        SensorFusion sensorFusion;
        sensorFusion.SetGravityEnabled(true);
        sensorFusion.SetPredictionEnabled(false);
        sensorFusion.SetYawCorrectionEnabled(false);
        sensorFusion.AttachToSensor(ovrSensor);

        for (int i = 0; i < 10; ++i) {
            Quatf orientation = sensorFusion.GetOrientation();
            float roll, pitch, yaw;
            orientation.GetEulerAngles<
                Axis::Axis_Z, Axis::Axis_X, Axis::Axis_Y,
                RotateDirection::Rotate_CCW,
                HandedSystem::Handed_R
            >(&roll, &pitch, &yaw);

            SAY("Current orientation - roll %0.2f, pitch %0.2f, yaw %0.2f",
                    roll * RADIANS_TO_DEGREES,
                    pitch  * RADIANS_TO_DEGREES,
                    yaw * RADIANS_TO_DEGREES);
            Platform::sleepMillis(1000);
        }

        sensorFusion.AttachToSensor(nullptr);
        return 0;
    }
};

RUN_OVR_APP(Tracker);
