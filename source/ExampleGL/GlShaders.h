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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <set>
#include <memory>

namespace GL {

class shader_error : public std::runtime_error
{
public:
  explicit shader_error(const std::string & __arg) : std::runtime_error(__arg) {}
  virtual ~shader_error() {}
};

struct Var {
    std::string name;
    GLint size;
    GLenum type;
    GLuint id;
    GLint location;

    // for map usage
    Var()
            : location(0), type(0), size(0), id(0) {
    }
    Var(bool uniform, GLuint program, GLuint id) {
        this->id = id;
        static GLchar MY_BUFFER[8192];
        GLsizei bufSize = 8192;
        if (uniform) {
            glGetActiveUniform(program, id, bufSize, &bufSize, &size, &type, MY_BUFFER);
        } else {
            glGetActiveAttrib(program, id, bufSize, &bufSize, &size, &type, MY_BUFFER);
        }
        name = std::string(MY_BUFFER, bufSize);
        if (uniform) {
            location = glGetUniformLocation(program, name.c_str());
        } else {
            location = glGetAttribLocation(program, name.c_str());
        }
    }
};

class Shader {
    friend class Program;
    GLenum type;
    GLuint shader;
    std::string source;

public:
    Shader(GLenum type, const std::string & source)
            : type(type), shader(0), source(source) {

        // Create the shader object
        GLuint newShader = glCreateShader(type);
        if (newShader == 0) {
            throw std::runtime_error("Unable to create shader object");
        }

        {
            static const char * c_str;
            // std::string shaderSrc = Files::read(fileName);
            // Load the shader source
            glShaderSource(newShader, 1, &(c_str = source.c_str()), NULL);
        }

        // Compile the shader
        glCompileShader(newShader);

        // Check the compile status
        GLint compiled;
        glGetShaderiv(newShader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            std::string log = getLog(newShader);
            SAY(log.c_str());
            throw std::runtime_error("Failed to compile shader " + log);
        }

        if (0 != shader) {
            glDeleteShader(shader);
        }
        shader = newShader;
    }

    virtual ~Shader() {
        glDeleteShader(shader);
    }

    static std::string getLog(GLuint shader) {
        std::string log;
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            char* infoLog = new char[infoLen];
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            log = std::string(infoLog);
            delete[] infoLog;
        }
        return log;
    }
};

typedef std::shared_ptr<Shader> ShaderPtr;

namespace Attribute { enum {
    Position = 0,
    Normal = 2,
    Color = 3,
    SecondaryColor = 4,
    FogCoord = 5,
    TexCoord0 = 8,
    TexCoord1 = 9,
    TexCoord2 = 10,
    TexCoord3 = 11,
    TexCoord4 = 12,
    TexCoord5 = 13,
    UserDefined = 16,
}; }

namespace Uniform { enum {
    Projection = 0,
    ModelView = 2,
    Color = 3,
    Texture0 = 8,
    Texture1 = 9,
    Texture2 = 10,
    Texture3 = 11,
    Texture4 = 12,
    Texture5 = 13,
    LightAmbient = 15,
    LightPositions = 16,
    LightColors = 24,
    UserDefined = 32,
}; }

class Program {
    typedef std::map<std::string, Var> Map;
    typedef std::set<std::string> Set;

    ShaderPtr vs;
    ShaderPtr fs;
    GLuint program;
    Map uniforms;
    Map attributes;
    Set configuredUniforms;
public:
    GLint getUniformLocation(const std::string & name) const  {
        return uniforms.count(name) ? uniforms.at(name).location : -1;
    }

    GLint getAttributeLocation(const std::string & name) const  {
        return attributes.count(name) ? attributes.at(name).location : -1;
    }

public:
    static std::string getLog(GLuint program) {
        std::string log;
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            char* infoLog = new char[infoLen];
            glGetProgramInfoLog(program, infoLen, NULL, infoLog);
            log = std::string(infoLog);
            delete[] infoLog;
        }
        return log;
    }

public:
    Program(const ShaderPtr & vs, const ShaderPtr & fs)
            : vs(vs), fs(fs), program(0) {
        if (0 != program) {
            glDeleteProgram(program);
            program = 0;
        }

        // Create the program object
        program = glCreateProgram();

        if (program == 0)
            throw std::runtime_error("Failed to allocate GL program");

        glAttachShader(program, vs->shader);
        glAttachShader(program, fs->shader);

        // Bind vPosition to attribute 0
        glBindAttribLocation(program, 0, "vPosition");

        // Link the newProgram
        glLinkProgram(program);

        // Check the link status
        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            throw std::runtime_error(getLog(program));
        }

        int numVars;
        glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numVars);
        for (int i = 0; i < numVars; ++i) {
            Var var(false, program, i);
            SAY("Found attribute %s at location %d", var.name.c_str(), var.location);
            attributes[var.name] = var;
        }
        glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numVars);
        for (int i = 0; i < numVars; ++i) {
            Var var(true, program, i);
            SAY("Found uniform %s at location %d", var.name.c_str(), var.location);
            uniforms[var.name] = var;
        }
    }

    virtual ~Program() {
        glDeleteProgram(program);
    }

    void setUniform1f(const std::string & name, GLfloat a) {
        configuredUniforms.insert(name);
        GLint location = getUniformLocation(name);
        glUniform1f(location, a);
    }

    void setUniform1i(const std::string & name, GLuint a) {
        configuredUniforms.insert(name);
        GLint location = getUniformLocation(name);
        glUniform1i(location, a);
    }

    void setUniform2f(const std::string & name, const glm::vec2 & a) {
        configuredUniforms.insert(name);
        GLint location = getUniformLocation(name);
        glUniform2f(location, a.x, a.y);
    }

    void setUniform4f(const std::string & name, const glm::vec4 & a) {
        configuredUniforms.insert(name);
        GLint location = getUniformLocation(name);
        glUniform4f(location, a.x, a.y, a.z, a.w);
    }

    void setUniform4fv(const std::string & name, const std::vector<glm::vec4> & a) {
        configuredUniforms.insert(name);
        GLint location = getUniformLocation(name);
        glUniform4fv(location, (GLsizei) a.size(), &(a[0][0]));
    }

    void setUniform3f(const std::string & name, const glm::vec3 & a) {
        configuredUniforms.insert(name);
        GLint location = getUniformLocation(name);
        glUniform3f(location, a.x, a.y, a.z);
    }

    void setUniform4x4f(const std::string & name, const glm::mat4 & a) {
        configuredUniforms.insert(name);
        GLint location = getUniformLocation(name);
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(a));
    }


    void checkConfigured() {
        Set existingUniforms;
        // Option #2
        std::for_each(std::begin(uniforms), std::end(uniforms), [&] (Map::const_reference element)
        {
            existingUniforms.insert(element.first);
        });

        std::for_each(std::begin(configuredUniforms), std::end(configuredUniforms), [&] (Set::const_reference element)
        {
            existingUniforms.erase(element);
        });

        assert(existingUniforms.size() == 0);
    }

    void use() {
        glUseProgram(program);
    }

    static void clear() {
        glUseProgram(0);
    }
};

typedef std::shared_ptr<Program> ProgramPtr;




} // GL
