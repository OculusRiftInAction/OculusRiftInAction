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
using namespace std;

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

void Platform::sleepMillis(int millis) {
#ifdef WIN32
  Sleep(millis);
#else
  usleep(millis * 1000);
#endif
}

long Platform::elapsedMillis() {
#ifdef WIN32
  static long start = GetTickCount();
  return GetTickCount() - start;
#else
  timeval time;
  gettimeofday(&time, NULL);
  long millis = (time.tv_sec * 1000) + (time.tv_usec / 1000);
  static long start = millis;
  return millis - start;
#endif
}

static const size_t BUFFER_SIZE = 8192;

void Platform::fail(const char * file, int line, const char * message, ...) {
  static char ERROR_BUFFER1[BUFFER_SIZE];
  static char ERROR_BUFFER2[BUFFER_SIZE];
  va_list arg;
  va_start(arg, message);
  vsnprintf(ERROR_BUFFER1, BUFFER_SIZE, message, arg);
  va_end(arg);
  snprintf(ERROR_BUFFER2, BUFFER_SIZE, "FATAL %s (%d): %s", file, line,
      ERROR_BUFFER1);
  std::string error(ERROR_BUFFER2);
  std::cerr << error << std::endl;
  // If you got here, something's pretty wrong
#ifdef WIN32
  DebugBreak();
#endif
  assert(0);
  throw error;
}

void Platform::say(std::ostream & out, const char * message, ...) {
  static char SAY_BUFFER[BUFFER_SIZE];
  va_list arg;
  va_start(arg, message);
  vsnprintf(SAY_BUFFER, BUFFER_SIZE, message, arg);
  va_end(arg);
#ifdef WIN32
  if (NULL == GetConsoleWindow()) {
    OutputDebugStringA(SAY_BUFFER);
	OutputDebugStringA("\n");
  }
#endif
  out << std::string(SAY_BUFFER) << std::endl;
}



