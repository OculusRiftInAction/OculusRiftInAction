/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Bradley Austin Davis. All Rights reserved.

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

#include "QtCommon.h"
#include "Application.h"
#include "MainWindow.h"
#include <qtextstream.h>

extern MainWindow * riftRenderWidget;

MAIN_DECL {
  try {
#ifdef USE_RIFT
    ovr_Initialize(nullptr);
#endif

    QT_APP_WITH_ARGS(ShadertoyApp);
    int result = app.exec();
    app.destroyWindow();

    ovr_Shutdown();

    return result;
  } catch (std::exception & error) {
    SAY_ERR(error.what());
  } catch (const std::string & error) {
    SAY_ERR(error.c_str());
  }
  return -1;
}





#if 0
SiHdl si;
SiGetEventData getEventData;
bool nativeEventFilter(const QByteArray & eventType, void * message, long * result) {
  // On Windows, eventType is set to "windows_generic_MSG" for messages sent
  // to toplevel windows, and "windows_dispatcher_MSG" for system - wide
  // messages such as messages from a registered hot key.In both cases, the
  // message can be casted to a MSG pointer.The result pointer is only used
  // on Windows, and corresponds to the LRESULT pointer.
  MSG & msg = *(MSG*)message;
  SiSpwEvent siEvent;
  vec3 tr, ro;
  long * md = siEvent.u.spwData.mData;
  SiGetEventWinInit(&getEventData, msg.message, msg.wParam, msg.lParam);
  if (SiGetEvent(si, 0, &getEventData, &siEvent) == SI_IS_EVENT) {
    switch (siEvent.type) {
    case SI_MOTION_EVENT:
      emit sixDof(
        vec3(md[SI_TX], md[SI_TY], -md[SI_TZ]),
        vec3(md[SI_RX], md[SI_RY], md[SI_RZ]));
      break;

    default:
      break;
    }
    return true;
  }
  return false;
}

SpwRetVal result = SiInitialize();
int cnt = SiGetNumDevices();
SiDevID devId = SiDeviceIndex(0);
SiOpenData siData;
SiOpenWinInit(&siData, (HWND)riftRenderWidget.effectiveWinId());
si = SiOpen("app", devId, SI_NO_MASK, SI_EVENT, &siData);
installNativeEventFilter(this);

#endif



/*
yes:
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/Xlf3D8.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/XsjXR1.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/XslXW2.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ld2GRz.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldSGRW.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldfGzr.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/4slGzn.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldj3Dm.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/lsl3W2.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/lts3Wn.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/XdBSzd.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/MsBGRh.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/4ds3zn.json"

maybe:

Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/Xll3Wn.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/Xsl3Rl.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldSGzR.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldX3Ws.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldsGWB.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/lsBXD1.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/Xds3zN.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/4sl3zn.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/4sjXzG.json"

debug:
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldjXD3.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/XdfXDB.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/Msl3Rr.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/4sXGDs.json"

*/
