#include <OVR.h>
#include "Common.h"

class Example_SdkInit {
public:

  virtual int run() {
    SAY("Initializing SDK");
    OVR::System::Init();

    SAY("Creating the device manager");
    OVR::Ptr<OVR::DeviceManager> ovrManager =
        *OVR::DeviceManager::Create();

    SAY("Releasing the device manager");
    ovrManager = nullptr;

    SAY("De-initializing the SDK");
    OVR::System::Destroy();

    SAY("Exiting");
    return 0;
  }
};

RUN_APP(Example_SdkInit);
