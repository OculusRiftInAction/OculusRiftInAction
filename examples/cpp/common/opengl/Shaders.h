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

typedef oglplus::Uniform<mat4> Mat4Uniform;
typedef std::shared_ptr<oglplus::VertexShader> VertexShaderPtr;
typedef std::shared_ptr<oglplus::FragmentShader> FragmentShaderPtr;
typedef std::shared_ptr<oglplus::Program> ProgramPtr;
typedef std::map<std::string, GLuint> UniformMap;

namespace oria {
  ProgramPtr loadProgram(Resource vs, Resource fs);
  ProgramPtr loadProgram(const std::string & vsFile, const std::string & fsFile);
  UniformMap getActiveUniforms(ProgramPtr & program);
}
