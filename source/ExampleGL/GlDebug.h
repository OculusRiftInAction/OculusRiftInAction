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

#ifndef GL_ZERO
#error "You must include the GL headers before including this file"
#endif

#ifdef WIN32
#include <Windows.h>
#endif

#include <stdexcept>
#include <iostream>
#include <ostream>
#include <cstdarg>
#include <string>
#include <cstring>

#define GL_CHECK_ERROR  { \
    GLenum error = glGetError(); \
    if (error != 0) { \
        throw std::runtime_error("OpenGL error code %d"); \
    } \
} \

#ifdef WIN32 
#define snprintf _snprintf
#endif

namespace GL {

class Debugger {
    static const size_t BUFFER_SIZE = 8192;
public:
    static void fail(const char * file, int line, const char * message, ...) {
        static char ERROR_BUFFER1[BUFFER_SIZE];
        static char ERROR_BUFFER2[BUFFER_SIZE];
        va_list arg;
        va_start(arg, message);
        vsnprintf(ERROR_BUFFER1, BUFFER_SIZE, message, arg);
        va_end(arg);
        snprintf(ERROR_BUFFER2, BUFFER_SIZE, "FATAL %s (%d): %s", file, line, ERROR_BUFFER1);
        std::string error(ERROR_BUFFER2);
        // If you got here, something's pretty wrong
    #ifdef WIN32
        DebugBreak();
    #endif
        assert(0);
        throw error;
    }

    static void say(std::ostream & out, const char * message, ...) {
        static char SAY_BUFFER[BUFFER_SIZE];
        va_list arg;
        va_start(arg, message);
        vsnprintf(SAY_BUFFER, BUFFER_SIZE, message, arg);
        va_end(arg);
    #ifdef WIN32
        if (NULL != GetConsoleWindow()) {
            OutputDebugStringA(SAY_BUFFER);
        }
    #endif
        out << std::string(SAY_BUFFER) << std::endl;
    }
};

}

#define FAIL(...) GL::Debugger::fail(__FILE__, __LINE__, __VA_ARGS__)

#define SAY(...) GL::Debugger::say(std::cout, __VA_ARGS__)

#define SAY_ERR(...) GL::Debugger::say(std::cerr, __VA_ARGS__)

