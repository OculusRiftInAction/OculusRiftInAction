#include "Common.h"

class Example_SdkInit {
public:

  virtual int run() {
    SAY("Initializing SDK");
    ovr_Initialize();

    int hmdCount = ovr_Detect();
    SAY("Found %d connected Rift device(s)", hmdCount);
    for (int i = 0; i < hmdCount; ++i) {
      ovrHmd hmd = ovr_Create(i);
      SAY(hmd->ProductName);
      ovr_Destroy(hmd);
    }

    ovrHmd hmd = ovr_CreateDebug(ovr_DK2);
    SAY(hmd->ProductName);
    ovr_Destroy(hmd);

    ovr_Shutdown();

    SAY("Exiting");
    return 0;
  }
};

RUN_APP(Example_SdkInit);
