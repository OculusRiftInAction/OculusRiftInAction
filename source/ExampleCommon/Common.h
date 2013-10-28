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
#ifdef WIN32
#include <Windows.h>
#endif

#include <string>
#include <cassert>
#include <iostream>
#include <sstream>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <OVR.h>
#undef new

#ifdef __APPLE__
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#pragma thanks_obama
#endif

#include "GlBuffers.h"
#include "GlFrameBuffer.h"
#include "GlStacks.h"
#include "GlShaders.h"
#include "GlGeometry.h"
#include "GlMethods.h"
#include "GlLighting.h"

#include "ExampleResources.h"

#include "Font.h"
#include "Rift.h"

#include "GlUtils.h"
#include "GlfwApp.h"
#include "ExampleResources.h"


namespace RiftExamples {

class Platform {
public:
//    static std::string loadResource(Resource resource);
    static const std::string & getResourcePath(Resource resource);
    static GL::GeometryPtr getGeometry(Resource resource);
    static GL::GeometryPtr getGeometry(const std::string & file);
    static GL::ProgramPtr getProgram(Resource resource);
    static GL::ProgramPtr getProgram(Resource vertexResource, Resource fragmentResource);
    static GL::ProgramPtr getProgram(const std::string & vs, const std::string & fs);

    static Text::FontPtr getFont(Resource resource);
    static Text::FontPtr getFont(const std::string & file);
    static Text::FontPtr getDefaultFont();

    static void sleepMillis(int millis);
    static long elapsedMillis();
};

}
#ifndef PI
#define PI 3.14159265f
#endif

#ifndef RADIANS_TO_DEGREES
#define RADIANS_TO_DEGREES (180.0f / PI)
#endif

#ifndef DEGREES_TO_RADIANS
#define DEGREES_TO_RADIANS (PI / 180.0f)
#endif

// Windows has a non-standard main function prototype
#ifdef WIN32
    #define MAIN_DECL int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
    #define MAIN_DECL int main(int argc, char ** argv)
#endif

// Combine some macros together to create a single macro
// to run an example app
#define RUN_APP(AppClass) \
    MAIN_DECL { \
        try { \
            return AppClass().run(); \
        } catch (const std::string & error) { \
            SAY(error.c_str()); \
        } \
        return -1; \
    }

