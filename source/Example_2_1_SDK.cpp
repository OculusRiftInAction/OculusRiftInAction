#include "Common.h"

class Example_SdkInit {
public:

  virtual int run() {
    SAY("Initializing SDK");
    ovr_Initialize();

    SAY("De-initializing the SDK");
    ovr_Shutdown();

    SAY("Exiting");
    return 0;
  }
};

RUN_APP(Example_SdkInit);
