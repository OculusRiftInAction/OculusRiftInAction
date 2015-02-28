/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.

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

#include "Common.h"

RiftManagerApp::RiftManagerApp(ovrHmdType defaultHmdType) {
  hmd = ovrHmd_Create(0);
  if (nullptr == hmd) {
    hmd = ovrHmd_CreateDebug(defaultHmdType);
    hmdDesktopPosition = glm::ivec2(100, 100);
  } else {
    hmdDesktopPosition = glm::ivec2(hmd->WindowsPos.x, hmd->WindowsPos.y);
  }
  hmdNativeResolution = glm::ivec2(hmd->Resolution.w, hmd->Resolution.h);
}

RiftManagerApp::~RiftManagerApp() {
  if (hmd) {
    ovrHmd_Destroy(hmd);
    hmd = nullptr;
  }
}

int RiftManagerApp::getEnabledCaps() {
  return ovrHmd_GetEnabledCaps(hmd);
}

void RiftManagerApp::enableCaps(int caps) {
  ovrHmd_SetEnabledCaps(hmd, getEnabledCaps() | caps);
}

void RiftManagerApp::toggleCaps(ovrHmdCaps cap) {
  if (cap & getEnabledCaps()) {
    disableCaps(cap);
  } else {
    enableCaps(cap);
  }
}

void RiftManagerApp::disableCaps(int caps) {
  ovrHmd_SetEnabledCaps(hmd, getEnabledCaps() & ~caps);
}
