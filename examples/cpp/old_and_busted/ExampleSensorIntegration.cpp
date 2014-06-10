#include "Common.h"
#include <boost/thread/mutex.hpp>
#include <boost/circular_buffer.hpp>

class SensorRecording : public OVR::SensorDevice, OVR::MessageHandler
{
  OVR::SensorDevice::CoordinateFrame coordframe;
  OVR::SensorRange range;
  unsigned rateHz;
  typedef std::vector<OVR::MessageBodyFrame>  Vector;
  Vector messages;
  OVR::Ptr<OVR::SensorDevice> sensorPtr;

public:

  virtual bool SetFeatureReport(OVR::UByte* data, OVR::UInt32 length) {
    return true;
  }

  virtual bool GetFeatureReport(OVR::UByte* data, OVR::UInt32 length) {
    return true;
  }


  virtual void            SetCoordinateFrame(OVR::SensorDevice::CoordinateFrame coordframe) {
    throw std::runtime_error("Unable to set properties on SensorRecording");
  }
  virtual CoordinateFrame GetCoordinateFrame() const {
    return coordframe;
  }

  virtual void        SetReportRate(unsigned rateHz) {
    throw std::runtime_error("Unable to set properties on SensorRecording");
  }

  virtual unsigned    GetReportRate() const {
    return rateHz;
  }

  virtual bool       SetRange(const OVR::SensorRange& range, bool waitFlag = false)  {
    throw std::runtime_error("Unable to set properties on SensorRecording");
  }

  virtual void       GetRange(OVR::SensorRange* range) const {
    *range = this->range;
  }

  virtual OVR::DeviceCommon*   getDeviceCommon() const {
    return (OVR::DeviceCommon*)this;
  }

  virtual void OnMessage(const OVR::Message& rawmsg) {
    const OVR::MessageBodyFrame & msg = 
      static_cast<const OVR::MessageBodyFrame&>(rawmsg);
    messages.push_back(msg);
  }

  virtual bool SupportsMessageType(OVR::MessageType type) const {
    return type == OVR::MessageType::Message_BodyFrame;
  }

  void record(OVR::Ptr<OVR::SensorDevice> source) {
    source->GetRange(&range);
    coordframe = source->GetCoordinateFrame();
    rateHz = source->GetReportRate();
    source->SetMessageHandler(this);
    sensorPtr = source;
  }

  void stopRecording() {
    sensorPtr.Clear();
  }
};


struct SensorData {
    const glm::vec3 acc;
    const glm::vec3 rot;
    const glm::quat orient;
    const float elapsed;

    SensorData(const OVR::MessageBodyFrame & msg, const glm::quat & q) :
            acc(Rift::fromOvr(msg.Acceleration)),
            rot(Rift::fromOvr(msg.RotationRate)),
            elapsed(msg.TimeDelta), orient(q) {
    }
};

static const glm::dvec3 GRAVITY = glm::dvec3(0, 9.811f, 0);


class Example05 : public OVR::MessageHandler {
protected:
    OVR::Ptr<OVR::SensorDevice> ovrSensorDevice;
    OVR::Ptr<OVR::DeviceManager> ovrDeviceManager;
    OVR::SensorFusion ovrSensorFusion;
    typedef std::vector<SensorData> SensorBuffer;
    SensorBuffer sensorBuffer;
    boost::mutex sensorBufferMutex;
    glm::dvec3 velocity;
    glm::dvec3 position;
    float gravity;
    glm::vec3 rot;
    glm::vec3 acc;

public:

    Example05() {
        ovrDeviceManager = *OVR::DeviceManager::Create();
        ovrSensorDevice = *ovrDeviceManager->EnumerateDevices<
                OVR::SensorDevice>().CreateDevice();

        if (!ovrSensorDevice) {
            FAIL("Unable to detect Rift head tracker");
        }
        ovrSensorFusion.AttachToSensor(ovrSensorDevice);
        ovrSensorFusion.SetGravityEnabled(true);
        ovrSensorFusion.SetPredictionEnabled(true);
        ovrSensorFusion.SetDelegateMessageHandler(this);
    }

    virtual void OnMessage(const OVR::Message& rawmsg) {
      const OVR::MessageBodyFrame & msg = static_cast<const OVR::MessageBodyFrame&>(rawmsg);
      {
        boost::unique_lock<boost::mutex> lock(sensorBufferMutex);
        sensorBuffer.push_back( SensorData(msg, Rift::fromOvr( ovrSensorFusion.GetOrientation())));
      }
    }

    virtual bool SupportsMessageType(OVR::MessageType type) const {
        return type == OVR::MessageType::Message_BodyFrame;
    }

    virtual void update() {
      SensorBuffer sensorBufferCopy;
      {
        boost::unique_lock<boost::mutex> lock(sensorBufferMutex);
        sensorBufferCopy.swap(sensorBuffer);
      }
      static int count;
      std::for_each(sensorBufferCopy.begin(), sensorBufferCopy.end(),
        [&](const SensorData & reading) {
            glm::dvec3 worldAcc = glm::dvec3(glm::inverse(reading.orient) * reading.acc);
            gravity = glm::length(worldAcc);
            worldAcc -= GRAVITY;
            glm::dvec3 deltaV = worldAcc * (double)reading.elapsed;
            velocity += deltaV;
            glm::dvec3 delta = (velocity * (double)reading.elapsed);
            position += delta;
            rot = reading.rot;
            acc = reading.acc;
        });
    }

    virtual void draw() {
      static long start = Platform::elapsedMillis();
      static long lastLog = start;
      long now = Platform::elapsedMillis();
      Platform::sleepMillis(15);
      if (now - lastLog >= 1000) {
        SAY("Time %06d \
          Gravity: %0.3f \n\
Rotation: %0.4f %0.4f %0.4f \n\
Accel:    %0.4f %0.4f %0.4f \n\
Velocity: %0.3f %0.3f %0.3f \n\
Position: %0.3f %0.3f %0.3f \n", 
            (int)(now - start), 
            gravity,
            rot.x, rot.y, rot.z,
            acc.x, acc.y, acc.z,
            velocity.x, velocity.y, velocity.z,
            position.x, position.y, position.z
        );
        lastLog = now;
      }
    }

    int run() {
      SensorRecording recording;
      while (1) {
        update();
        draw();
      }
      return 1;
    }

};

RUN_OVR_APP(Example05);
