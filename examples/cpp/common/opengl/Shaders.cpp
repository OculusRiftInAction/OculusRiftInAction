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
        Platform::getResourceString(vs),
        Platform::getResourceString(fs));
      // FIXME
      // Caching shaders is problematic, since it requires you to set ALL 
      // uniforms any time you use the shader, because you don't know if you're 
      // sharing the shader instance with something else in the program which 
      // is using the same shader for a different purpose, and with different 
      // uniforms.
      // Need to decide if it's better policy to cache shaders and expect full uniform 
      // initialization every time we use them or to prevent shader caching.  
      // For now it's disabled.  
      // programs[key] = result;
      return result;
    }
    return programs[key];
  }

  ProgramPtr loadProgram(const std::string & vsFile, const std::string & fsFile) {
    ProgramPtr result;
    compileProgram(result,
      oria::readFile(vsFile),
      oria::readFile(fsFile));
    return result;
  }

  UniformMap getActiveUniforms(ProgramPtr & program) {
    UniformMap activeUniforms;
    size_t uniformCount = program->ActiveUniforms().Size();
    for (size_t i = 0; i < uniformCount; ++i) {
      std::string name = program->ActiveUniforms().At(i).Name();
      activeUniforms[name] = program->ActiveUniforms().At(i).Index();
    }
    return activeUniforms;
  }

}
