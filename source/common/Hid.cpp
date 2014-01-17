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
#include "Hid.h"
/*
#include <libusb/libusb.h>

// HID Class-Specific Requests values. See section 7.2 of the HID specifications
#define HID_GET_REPORT                0x01
#define HID_GET_IDLE                  0x02
#define HID_GET_PROTOCOL              0x03
#define HID_SET_REPORT                0x09
#define HID_SET_IDLE                  0x0A
#define HID_SET_PROTOCOL              0x0B
#define HID_REPORT_TYPE_INPUT         0x01
#define HID_REPORT_TYPE_OUTPUT        0x02
#define HID_REPORT_TYPE_FEATURE       0x03
#define CALL_CHECK(fcall) do { int r=fcall; if (r < 0) FAIL("%d %s", r, strerror(-r)); } while (0);


namespace hid {
void FeatureReport::read(libusb_device_handle * handle) {
  CALL_CHECK(
      libusb_control_transfer(handle,
          LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS
              | LIBUSB_RECIPIENT_INTERFACE,
          HID_GET_REPORT, (HID_REPORT_TYPE_FEATURE << 8) | REPORT_ID, 0,
          buffer(),
          SIZE, 5000));
}

void FeatureReport::write(libusb_device_handle * handle) {
  for (int i = 0; i < 10; ++i) {
    int r = libusb_control_transfer(handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS
            | LIBUSB_RECIPIENT_INTERFACE,
        HID_SET_REPORT,
        (HID_REPORT_TYPE_FEATURE << 8) | REPORT_ID, 0,
        buffer(),
        SIZE, 5000);
    if (r > -1) {
      return;
    }
    if (r == LIBUSB_ERROR_PIPE) {
      libusb_clear_halt(handle, 0);
    }
  }
}
} // namespace hid
*/

