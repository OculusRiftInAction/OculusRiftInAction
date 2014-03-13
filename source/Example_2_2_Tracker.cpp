#include "Common.h"

class Tracker {
protected:
  OVR::Ptr<OVR::DeviceManager> ovrManager;

public:
    Tracker() {
      ovrManager = *OVR::DeviceManager::Create();
        if (!ovrManager) {
            FAIL("Unable to create OVR device manager");
        }
    }

    int run() {
      OVR::Ptr<OVR::SensorDevice> ovrSensor = *ovrManager->
        EnumerateDevices<OVR::SensorDevice>().CreateDevice();
        if (!ovrSensor) {
            SAY_ERR("Unable to detect Rift head tracker");
            return -1;
        }
        OVR::SensorFusion ovrSensorFusion;
        ovrSensorFusion.SetGravityEnabled(true);
        ovrSensorFusion.SetPredictionEnabled(false);
        ovrSensorFusion.SetYawCorrectionEnabled(false);
        ovrSensorFusion.AttachToSensor(ovrSensor);

        for (int i = 0; i < 10; ++i) {
          OVR::Quatf orientation = ovrSensorFusion.GetOrientation();
          float roll, pitch, yaw;

          orientation.GetEulerAngles<
            OVR::Axis::Axis_Z, OVR::Axis::Axis_X, OVR::Axis::Axis_Y,
            OVR::RotateDirection::Rotate_CCW,
            OVR::HandedSystem::Handed_R
          >(&roll, &pitch, &yaw);

          SAY("Current orientation - roll %0.2f, pitch %0.2f, yaw %0.2f",
                  roll * RADIANS_TO_DEGREES,
                  pitch  * RADIANS_TO_DEGREES,
                  yaw * RADIANS_TO_DEGREES);
          Platform::sleepMillis(1000);
        }
        ovrSensorFusion.AttachToSensor(nullptr);
        return 0;
    }
};

RUN_OVR_APP(Tracker);
