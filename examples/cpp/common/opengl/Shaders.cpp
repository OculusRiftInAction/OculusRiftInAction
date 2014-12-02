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
#include "Files.h"

namespace oria {

  void compileProgram(ProgramPtr & result, std::string vs, std::string fs) {
    using namespace oglplus;
    try {
      result = ProgramPtr(new Program());
      // attach the shaders to the program
      result->AttachShader(
        VertexShader()
        .Source(GLSLSource(vs))
        .Compile()
        );
      result->AttachShader(
        FragmentShader()
        .Source(GLSLSource(fs))
        .Compile()
        );
      result->Link();
    } catch (ProgramBuildError & err) {
      SAY_ERR((const char*)err.Message);
      result.reset();
    }
  }


  ProgramPtr loadProgram(Resource vs, Resource fs) {
    typedef std::unordered_map<std::string, ProgramPtr> ProgramMap;

    static ProgramMap programs;
    static bool registeredShutdown = false;
    if (!registeredShutdown) {
      Platform::addShutdownHook([&]{
        programs.clear();
      });
      registeredShutdown = true;
    }

    std::string key = Resources::getResourcePath(vs) + ":" +
      Resources::getResourcePath(fs);
    if (!programs.count(key)) {
      ProgramPtr result;
      compileProgram(result,
        Platform::getResourceData(vs),
        Platform::getResourceData(fs));
//      programs[key] = result;
      return result;
    }
    return programs[key];
  }

  ProgramPtr loadProgram(const std::string & vsFile, const std::string & fsFile) {
    ProgramPtr result;
    compileProgram(result,
      Files::read(vsFile),
      Files::read(fsFile));
    return result;
  }

}
