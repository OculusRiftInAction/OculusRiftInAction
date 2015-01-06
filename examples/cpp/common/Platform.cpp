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

#ifdef OS_WIN
#pragma warning (disable : 4996)
#include <Windows.h>
#define snprintf _snprintf
#else
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <cstdarg>
#endif

void Platform::sleepMillis(int millis) {
#ifdef OS_WIN
  Sleep(millis);
#else
  usleep(millis * 1000);
#endif
}

long Platform::elapsedMillis() {
#ifdef OS_WIN
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

float Platform::elapsedSeconds() {
  return (float)elapsedMillis() / 1000.0f;
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
#ifdef OS_WIN
  if (NULL == GetConsoleWindow()) {
    MessageBoxA(NULL, ERROR_BUFFER2, "Message", IDOK | MB_ICONERROR);
  }
  DebugBreak();
#endif
  // assert(0);
  throw std::runtime_error(error.c_str());
}

void Platform::say(std::ostream & out, const char * message, ...) {
  static char SAY_BUFFER[BUFFER_SIZE];
  va_list arg;
  va_start(arg, message);
  vsnprintf(SAY_BUFFER, BUFFER_SIZE, message, arg);
  va_end(arg);
#ifdef OS_WIN
  OutputDebugStringA(SAY_BUFFER);
  OutputDebugStringA("\n");
#endif
  out << std::string(SAY_BUFFER) << std::endl;
}

std::string Platform::getResourceString(Resource resource) {
  size_t size = Resources::getResourceSize(resource);
  char * data = new char[size];
  Resources::getResourceData(resource, data);
  std::string dataStr(data, size);
  delete[] data;
  return dataStr;
}

std::vector<uint8_t> Platform::getResourceByteVector(Resource resource) {
  size_t size = Resources::getResourceSize(resource);
  std::vector<uint8_t> data; 
  data.resize(size);
  Resources::getResourceData(resource, &data[0]);
  return data;
}

std::stringstream && Platform::getResourceStream(Resource resource) {
  std::string data = Platform::getResourceString(resource);
  std::stringstream result;
  result.str(data);
  return std::move(result);
}


std::string Platform::format(const char * fmt_str, ...) {
    int final_n, n = (int)strlen(fmt_str) * 2; /* reserve 2 times as much as the length of the fmt_str */
    std::string str;
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]); /* wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str);
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str, ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}



std::string Platform::replaceAll(const std::string & in, const std::string & from, const std::string & to) {
  std::string str(in);
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // ...
  }
  return str;
}


typedef std::vector<std::function<void()>> VecLambda;
VecLambda & getShutdownHooks() {
  static VecLambda hooks;
  return hooks;
}

void Platform::addShutdownHook(std::function<void()> f) {
  getShutdownHooks().push_back(f);
}

void Platform::runShutdownHooks() {
  VecLambda & hooks = getShutdownHooks();
  std::for_each(hooks.begin(), hooks.end(), [&](std::function<void()> f){
    f();
  });
}

void Platform::setThreadPriority(ThreadPriority priority) {
#ifdef OS_WIN
  int win32priority;
  switch (priority) {
  case LOW:
    win32priority = THREAD_PRIORITY_LOWEST;
    break;
  case MEDIUM:
    win32priority = THREAD_PRIORITY_NORMAL;
    break;
  case HIGH:
    win32priority = THREAD_PRIORITY_HIGHEST;
    break;
  }
  SetThreadPriority(GetCurrentThread(), win32priority);
#else 
  sched_param params;
  switch (priority) {
  case LOW:
    params.sched_priority = sched_get_priority_min(SCHED_FIFO);
    break;
  case MEDIUM:
    params.sched_priority = 0;
    break;
  case HIGH:
    params.sched_priority = sched_get_priority_max(SCHED_FIFO);
    break;
  }
  pthread_setschedparam(pthread_self(), SCHED_FIFO, &params);
#endif
}
