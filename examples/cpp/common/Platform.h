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

class Platform {

public:

  enum ThreadPriority {
    LOW,
    MEDIUM,
    HIGH
  };
  static void sleepMillis(int millis);
  static long elapsedMillis();
  static float elapsedSeconds();
  static void fail(const char * file, int line, const char * message, ...);
  static void say(std::ostream & out, const char * message, ...);
  static std::string format(const char * formatString, ...);
  static std::string getResourceString(Resource resource);
  static std::vector<uint8_t> getResourceByteVector(Resource resource);
  static std::stringstream && getResourceStream(Resource resource);

  static std::string replaceAll(const std::string & in, const std::string & from, const std::string & to);
  static void setThreadPriority(ThreadPriority priority = MEDIUM);

  static void addShutdownHook(std::function<void()> f);
  static void runShutdownHooks();
};

#define FAIL(...) Platform::fail(__FILE__, __LINE__, __VA_ARGS__)
#define SAY(...) Platform::say(std::cout, __VA_ARGS__)
#define SAY_ERR(...) Platform::say(std::cerr, __VA_ARGS__)
