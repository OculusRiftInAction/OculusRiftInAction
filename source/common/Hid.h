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

#pragma once

struct libusb_device_handle;

namespace hid {

  #pragma pack(push, 1)
  struct FeatureReport {
    const size_t SIZE;
    const uint8_t REPORT_ID;
    uint8_t reportId;
    uint16_t commandId;

    FeatureReport(uint8_t reportId, size_t size) :
        SIZE(size), REPORT_ID(reportId) {}

    uint8_t * buffer() {
      return &reportId;
    }

    void read(libusb_device_handle * handle);
    void write(libusb_device_handle * handle);
  };
  #pragma pack(pop)

}
