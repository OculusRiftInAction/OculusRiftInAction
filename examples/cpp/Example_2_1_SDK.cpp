#include "Common.h"

class Example_SdkInit {
public:

  virtual int run() {
    SAY("Initializing SDK");
    ovr_Initialize();

    int hmdCount = ovrHmd_Detect();
    SAY("Found %d connected Rift device(s)", hmdCount);
    for (int i = 0; i < hmdCount; ++i) {
      ovrHmd hmd = ovrHmd_Create(i);
      SAY(hmd->ProductName);
      ovrHmd_Destroy(hmd);
    }

    ovrHmd hmd = ovrHmd_CreateDebug(ovrHmd_DK2);
    SAY(hmd->ProductName);
    ovrHmd_Destroy(hmd);


    ovr_Shutdown();

    SAY("Exiting");
    return 0;
  }
};

RUN_APP(Example_SdkInit);
