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
#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <regex>
#include <istream>
#include <cstring>
#include <map>
#include <openctmpp.h>
#include "Files.h"
using namespace std;

#ifdef WIN32
    #include <Windows.h>
#else
	#include <sys/time.h>
	#include <unistd.h>
#endif

#ifdef __APPLE__
    #include "CoreFoundation/CFBundle.h"
#endif

#include <istream>
#include <sstream>
#include <fstream>

namespace RiftExamples {

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

#define BUFFER_SIZE 8192

#ifdef WIN32
#define snprintf sprintf_s
#pragma warning(disable:4996)
//#define _CRT_SECURE_NO_WARNINGS
#endif



static string slurpStream(istream& in) {
    stringstream sstr;
    sstr << in.rdbuf();
    string result = sstr.str();
    return result;
}

string loadFile(const string & in) {
    ifstream ins(in.c_str(), ios::binary);
    if (!ins) {
        std::cerr << "Failed to load resource " << in << std::endl;
    }
    assert(ins);
    return slurpStream(ins);
}


using namespace Text;
using namespace GL;
    const string & getResourcePath(Resource res);

    Text::FontPtr Platform::getFont(Resource fontName) {
        return getFont(getResourcePath(fontName));
    }

    Text::FontPtr Platform::getFont(const string & file) {
        static map<string, FontPtr> fonts;
        if (fonts.find(file) == fonts.end()) {
            FontPtr result(new Font());
            std::string fontData = Files::read(file);
            size_t size = fontData.size();
            result->read((const void*)fontData.data(), fontData.size());
            fonts[file] = result;
        }
        return fonts[file];
    }

    Text::FontPtr Platform::getDefaultFont() {
        return getFont(Resource::FONTS_INCONSOLATA_MEDIUM_SDFF);
    }

    static CTMuint CTMreadfn(void * aBuf, CTMuint aCount, void * aUserData) {
        std::istream & instream = *(std::istream *) aUserData;
        return instream.readsome((char*) aBuf, aCount);
    }

    class CtmFloatLoader : public GL::GlBufferLoader {
        CTMimporter & importer;
        float * data;

    public:
        CtmFloatLoader(CTMimporter & importer) :
                importer(importer), data(nullptr) {
        }

        virtual ~CtmFloatLoader() {
            if (data != nullptr) {
                delete[] data;
            }
        }

        GLsizeiptr getSize() {
            int perVertexFloats = Geometry::VERTEX_ATTRIBUTE_SIZE;
            int hasNormals = importer.GetInteger(CTM_HAS_NORMALS);
            if (hasNormals) {
                perVertexFloats *= 2;
            }
            int vertices = importer.GetInteger(CTM_VERTEX_COUNT);
            return sizeof(float) * perVertexFloats * vertices;
        }

        const GLvoid* getData() {
            if (nullptr == data) {
                int perVertexFloats = Geometry::VERTEX_ATTRIBUTE_SIZE;
                int hasNormals = importer.GetInteger(CTM_HAS_NORMALS);
                if (hasNormals) {
                    perVertexFloats *= 2;
                }
                int vertices = importer.GetInteger(CTM_VERTEX_COUNT);
                data = new float[getSize()];
                const float * ctmData = importer.GetFloatArray(CTM_VERTICES);
                for (int i = 0; i < vertices; ++i) {
                    size_t dataOffset = i * perVertexFloats;
                    size_t ctmOffset = i * 3;
                    memcpy(data + dataOffset, ctmData + ctmOffset, sizeof(float) * 3);
                }
                if (hasNormals) {
                    ctmData = importer.GetFloatArray(CTM_NORMALS);
                    for (int i = 0; i < vertices; ++i) {
                        size_t dataOffset = (i * perVertexFloats) + 4;
                        size_t ctmOffset = i * 3;
                        memcpy(data + dataOffset, ctmData + ctmOffset, sizeof(float) * 3);
                    }
                }
            }
            return (GLvoid*) data;
        }
    };


    class CtmIndexLoader : public GL::GlBufferLoader {
        CTMimporter & importer;
    public:
        CtmIndexLoader(CTMimporter & importer) :
                importer(importer) {
        }
        GLsizeiptr getSize() {
            return sizeof(int) * 3 * importer.GetInteger(CTM_TRIANGLE_COUNT);
        }

        const GLvoid* getData() {
            return (GLvoid*) importer.GetIntegerArray(CTM_INDICES);
        }
    };

    GeometryPtr Platform::getGeometry(Resource res) {
        return getGeometry(getResourcePath(res));
    }

    GeometryPtr Platform::getGeometry(const string & file) {
        static std::map<string, GeometryPtr> cache;
        if (!cache.count(file)) {
            CTMimporter importer;
            std::string data = Files::read(file);
            importer.LoadData(data);
            GL::VertexBufferPtr in_vertices(new VertexBuffer());
            GL::IndexBufferPtr in_indices(new IndexBuffer());
            (*in_vertices) << CtmFloatLoader(importer);
            (*in_indices) << CtmIndexLoader(importer);
            unsigned int triangles = importer.GetInteger(CTM_TRIANGLE_COUNT);
            unsigned int flags = importer.GetInteger(CTM_HAS_NORMALS) ?  GL::Geometry::HAS_NORMAL : 0;
            cache[file] = GeometryPtr(new Geometry(in_vertices, in_indices, triangles, flags));
        }
        return cache[file];
    }


    ProgramPtr Platform::getProgram(Resource  vs) {
        return getProgram(getResourcePath(vs),
                getResourcePath(static_cast<Resource>( vs - 1 ))
                );
    }
    ProgramPtr Platform::getProgram(Resource  vs, Resource  fs) {
        return getProgram(getResourcePath(vs), getResourcePath(fs));
    }

    template <GLenum TYPE> struct ShaderInfo {
        time_t modified;
        GL::ShaderPtr shader;

        bool valid(const string & file) {
            return shader && (Files::modified(file) <= modified);
        }

        void compile(const string & file) {
            SAY("Compiling shader file %s", file.c_str());
            shader = ShaderPtr(new Shader(TYPE, Files::read(file)));
            modified = Files::modified(file);

        }
        bool update(const string & file) {
            if (!valid(file)) {
                compile(file);
                return true;
            }
            return false;
        }
    };

    ProgramPtr Platform::getProgram(const string & vs, const string & fs) {
        typedef ShaderInfo<GL_VERTEX_SHADER> VShader;
        typedef ShaderInfo<GL_FRAGMENT_SHADER> FShader;
        typedef map<string, VShader> VMap;
        typedef map<string, FShader> FMap;
        static VMap vShaders;
        static FMap fShaders;
        VShader & vsi = vShaders[vs];
        FShader & fsi = fShaders[fs];
        bool relink = vsi.update(vs) | fsi.update(fs);
        typedef map<string, ProgramPtr> ProgramMap;
        string key = vs + ":" + fs;
        static ProgramMap programs;
        if (relink || programs.end() == programs.find(key)) {
            cerr << "Relinking " + key << endl;
            programs[key] = ProgramPtr(new Program(vsi.shader, fsi.shader));
        }
        return programs[key];
    }
}


/*
// Linux implmentation
#if !defined(WIN32) && !defined(__APPLE__)
    std::string baseResourcePath;

    void Platform::initBaseResourcePath(const std::string & executablePath) {
        string executable(executablePath);
        string::size_type sep = executable.rfind('/');
        if (sep != string::npos) {
            baseResourcePath = executable.substr(0, sep) + "/resources";
        }
    }

    string Platform::loadResource(const string& in) {
        return slurpFile(baseResourcePath + "/" + in);
    }

#elif defined(WIN32)

    string Platform::loadResource(const string& in) {
        static HMODULE module = GetModuleHandle(NULL);
        static std::regex space("\\s+");
        std::string loadTarget = std::regex_replace(in, space, "_");
        HRSRC res = FindResourceA(module, loadTarget.c_str(), "TEXTFILE");
        assert(res);
        HGLOBAL mem = LoadResource(module, res);
        assert(mem);
        DWORD size = SizeofResource(module, res);
        LPVOID data = LockResource(mem);
        assert(data);
        string result = string((const char*)data, size);
        FreeResource(mem);
        return result;
    }

#elif defined(__APPLE__)
    string Platform::loadResource(const string& in) {
        static CFBundleRef mainBundle = CFBundleGetMainBundle();
        assert(mainBundle);
        static char PATH_BUFFER[PATH_MAX];
        CFStringRef stringRef = CFStringCreateWithCString(NULL, in.c_str(), kCFStringEncodingASCII);
        assert(stringRef);
        CFURLRef resourceURL = CFBundleCopyResourceURL(mainBundle, stringRef, NULL, NULL);
        assert(resourceURL);
        auto result = CFURLGetFileSystemRepresentation(resourceURL, true, (UInt8*)PATH_BUFFER, PATH_MAX);
        assert(result);
        return slurpFile(string(PATH_BUFFER));
    }
#endif // WIN32

*/

//        class ProgramManager {
//        public:
//
//
//            void resetContext() {
//                programs.clear();
//            }
//
//            ProgramPtr & getActiveProgram() {
//                return activeProgram;
//            }
//
//            void setActiveProgram(Program * program) {
//                if (!program) {
//                    activeProgram.reset();
//                    return;
//                }
//                for (Itr itr = programs.begin(); itr != programs.end(); ++itr) {
//                    ProgramPtr & ptr = itr->second;
//                    if (ptr.get() == program) {
//                        activeProgram = ptr;
//                        return;
//                    }
//                }
//                // We couldn't find the program, this shouldn't be possible
//                throw exception();
//            }
//
//        } pm;
//
//        void Program::resetContext() {
//            pm.resetContext();
//        }
//        ProgramPtr & Program::getActiveProgram() {
//            return pm.getActiveProgram();
//        }
//
//        ProgramPtr & Program::getProgram(Resource vs) {
//            static bool init = true;
//            static map<Resource, Resource> resourcePairs;
//            if (init) {
//                init = false;
//                resourcePairs[Resource::SHADERS_SIMPLE_VS] =
//                        Resource::SHADERS_SIMPLE_FS;
//                resourcePairs[Resource::SHADERS_LIT_VS] =
//                        Resource::SHADERS_LIT_FS;
//                resourcePairs[Resource::SHADERS_TEXT_VS] =
//                        Resource::SHADERS_TEXT_FS;
//            }
//            assert(resourcePairs.count(vs));
//            return pm.getProgram(vs, resourcePairs[vs]);
//        }
//
//
//
//        ProgramPtr & Program::getProgram(Resource vs, Resource fs) {
//            return pm.getProgram(vs, fs);
//        }
