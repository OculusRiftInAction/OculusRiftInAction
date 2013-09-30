#if defined(WIN32)

    #include <GL/gl3w.h>

#else

    #include <sstream>
    #include <fstream>
    #define GL_GLEXT_PROTOTYPES

    #ifdef __APPLE__
        #include "CoreFoundation/CFBundle.h"
        #include <OpenGL/gl.h>
        #include <OpenGL/glext.h>
    #else
        #include <GL/gl.h>
        #include <GL/glext.h>
    #endif

#endif // WIN32

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#include <iostream>
#include <string>
#include <map>
#include <stdint.h>

#include "OVR.h"
#undef new

#ifdef WIN32

	long millis() {
		static long start = GetTickCount();
		return GetTickCount() - start;
	}

#else

	#include <sys/time.h>

	long millis() {
		timeval time;
		gettimeofday(&time, NULL);
		long millis = (time.tv_sec * 1000) + (time.tv_usec / 1000);
		static long start = millis;
		return millis - start;
	}

#endif

using namespace std;
using namespace OVR;
using namespace OVR::Util::Render;

// Some defines to make calculations below more transparent
#define TRIANGLES_PER_FACE 2
#define VERTICES_PER_TRIANGLE 3
#define VERTICES_PER_EDGE 2
#define FLOATS_PER_VERTEX 3

// Cube geometry
#define VERT_COUNT 8
#define FACE_COUNT 6
#define EDGE_COUNT 12

#define CUBE_SIZE 0.4f
#define CUBE_P (CUBE_SIZE / 2.0f)
#define CUBE_N (-1.0f * CUBE_P)

#define ON 1.0
#define PQ 0.25

#define RED 1, 0, 0
#define GREEN 0, 1, 0
#define BLUE 0, 0, 1
#define YELLOW 1, 1, 0
#define CYAN 0, 1, 1
#define MAGENTA 1, 0, 1

// How big do we want our renderbuffer
#define FRAMEBUFFER_OBJECT_SCALE 3

const glm::vec3 X_AXIS = glm::vec3(1.0f, 0.0f, 0.0f);
const glm::vec3 Y_AXIS = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 Z_AXIS = glm::vec3(0.0f, 0.0f, 1.0f);        // Animate the cube
const glm::vec3 CAMERA = glm::vec3(0.0f, 0.0f, 0.8f);
const glm::vec3 ORIGIN = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 UP = Y_AXIS;

// Vertices for a unit cube centered at the origin
const GLfloat CUBE_VERTEX_DATA[VERT_COUNT * FLOATS_PER_VERTEX] = {
    CUBE_N, CUBE_N, CUBE_N, // Vertex 0 position
    CUBE_P, CUBE_N, CUBE_N, // Vertex 1 position
    CUBE_P, CUBE_P, CUBE_N, // Vertex 2 position
    CUBE_N, CUBE_P, CUBE_N, // Vertex 3 position
    CUBE_N, CUBE_N, CUBE_P, // Vertex 4 position
    CUBE_P, CUBE_N, CUBE_P, // Vertex 5 position
    CUBE_P, CUBE_P, CUBE_P, // Vertex 6 position
    CUBE_N, CUBE_P, CUBE_P, // Vertex 7 position
};


const GLfloat CUBE_FACE_COLORS[] = {
    RED, 1,
    GREEN, 1,
    BLUE, 1,
    YELLOW, 1,
    CYAN, 1,
    MAGENTA, 1,
};

// 6 sides * 2 triangles * 3 vertices
const unsigned int CUBE_INDICES[FACE_COUNT * TRIANGLES_PER_FACE * VERTICES_PER_TRIANGLE ] = {
   0, 4, 5, 0, 5, 1, // Face 0
   1, 5, 6, 1, 6, 2, // Face 1
   2, 6, 7, 2, 7, 3, // Face 2
   3, 7, 4, 3, 4, 0, // Face 3
   4, 7, 6, 4, 6, 5, // Face 4
   3, 0, 1, 3, 1, 2  // Face 5
};

//
const unsigned int CUBE_WIRE_INDICES[EDGE_COUNT * VERTICES_PER_EDGE ] = {
   0, 1, 1, 2, 2, 3, 3, 0, // square
   4, 5, 5, 6, 6, 7, 7, 4, // facing square
   0, 4, 1, 5, 2, 6, 3, 7, // transverse lines
};

const GLfloat QUAD_VERTICES[] = {
    -1, -1, 0, 0,
     1, -1, 1, 0,
     1,  1, 1, 1,
    -1,  1, 0, 1,
};

const GLuint QUAD_INDICES[] = {
   2, 0, 3, 0, 1, 2,
};


#ifdef WIN32

    static string loadResource(const string& in) {
		static HMODULE module = GetModuleHandle(NULL);
		HRSRC res = FindResourceA(module, in.c_str(), "TEXTFILE");
		HGLOBAL mem = LoadResource(module, res);
		DWORD size = SizeofResource(module, res);
		LPVOID data = LockResource(mem);
		string result((const char*)data, size);
		FreeResource(mem);
		return result;
    }

#else

	static string slurp(ifstream& in) {
		stringstream sstr;
		sstr << in.rdbuf();
		string result = sstr.str();
		assert(!result.empty());
		return result;
	}

	static string slurpFile(const string & in) {
		ifstream ins(in.c_str());
		assert(ins);
		return slurp(ins);
	}

	#ifdef __APPLE__
        static string loadResource(const string& in) {
            static CFBundleRef mainBundle = CFBundleGetMainBundle();
            assert(mainBundle);

            CFStringRef stringRef = CFStringCreateWithCString(NULL, in.c_str(), kCFStringEncodingASCII);
            assert(stringRef);
            CFURLRef resourceURL = CFBundleCopyResourceURL(mainBundle, stringRef, NULL, NULL);
            assert(resourceURL);
            char *fileurl = new char[PATH_MAX];
            auto result = CFURLGetFileSystemRepresentation(resourceURL, true, (UInt8*)fileurl, PATH_MAX);
            assert(result);
            return slurpFile(string(fileurl));
        }

    #else
        string executableDirectory(".");

        static string loadResource(const string& in) {
            return slurpFile(executableDirectory + "/" + in);
        }

	#endif // __APPLE__

#endif // WIN32

// A small class to encapsulate loading of shaders into a GL program
class GLprogram {


    static string getProgramLog(GLuint program) {
        string log;
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            char* infoLog = new char[infoLen];
            glGetProgramInfoLog(program, infoLen, NULL, infoLog);
            log = string(infoLog);
            delete[] infoLog;
        }
        return log;
    }

    static string getShaderLog(GLuint shader) {
        string log;
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            char* infoLog = new char[infoLen];
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            log = string(infoLog);
            delete[] infoLog;
        }
        return log;
    }

    static GLuint compileShader(GLuint type, const string shaderSrc) {
        // Create the shader object
        GLuint shader = glCreateShader(type);
        assert(shader != 0);
        const char * srcPtr = shaderSrc.c_str();
        glShaderSource(shader, 1, &srcPtr, NULL);
        glCompileShader(shader);
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == 0) {
			string errorLog = getShaderLog(shader);
            cerr << errorLog << endl;
        }
        assert(compiled != 0);
        return shader;
    }

    static GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader) {
        GLuint program = glCreateProgram();
        assert(program != 0);
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        // Link the newProgram
        glLinkProgram(program);
        // Check the link status
        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked == 0) {
            cerr << getProgramLog(program) << endl;
        }
        assert(linked != 0);
        return program;
    }

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint program;
    typedef map<string, GLint> Map;
    Map attributes;
    Map uniforms;

public:
	GLprogram() : vertexShader(0), fragmentShader(0), program(0) { }

    void use() {
        glUseProgram(program);
    }

    void close() {
        if (0 != program) {
            glDeleteProgram(program);
            program = 0;
        }
        if (0 != vertexShader) {
            glDeleteShader(vertexShader);
        }
        if (0 != fragmentShader) {
            glDeleteShader(fragmentShader);
        }
    }

    void open(const string & name) {
        open(name + ".vs", name + ".fs");
    }

    void open(const string & vertexShaderFile, const string & fragmentShaderFile) {
        string source = loadResource(vertexShaderFile);
        vertexShader = compileShader(GL_VERTEX_SHADER, source);
        source = loadResource(fragmentShaderFile);
        fragmentShader = compileShader(GL_FRAGMENT_SHADER, source);
        program = linkProgram(vertexShader, fragmentShader);
        attributes.clear();
        static GLchar GL_OUTPUT_BUFFER[8192];
        int numVars;
        glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numVars);
        for (int i = 0; i < numVars; ++i) {
            GLsizei bufSize = 8192;
            GLsizei size; GLenum type;
            glGetActiveAttrib(program, i, bufSize, &bufSize, &size, &type, GL_OUTPUT_BUFFER);
            string name = string(GL_OUTPUT_BUFFER, bufSize);
            GLint location = glGetAttribLocation(program, name.c_str());
            attributes[name] = location;
            cout << "Found attribute " << name << " at location " << location << endl;
        }

        uniforms.clear();
        glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numVars);
        for (int i = 0; i < numVars; ++i) {
            GLsizei bufSize = 8192;
            GLsizei size;
            GLenum type;
            glGetActiveUniform(program, i, bufSize, &bufSize, &size, &type, GL_OUTPUT_BUFFER);
            string name = string(GL_OUTPUT_BUFFER, bufSize);
            GLint location = glGetUniformLocation(program, name.c_str());
            uniforms[name] = location;
            cout << "Found uniform " << name << " at location " << location << endl;
        }
    }

    GLint getUniformLocation(const string & uniform) const {
        auto itr = uniforms.find(uniform);
        if (uniforms.end() != itr) {
            return itr->second;
        }
        return -1;
    }

    GLint getAttributeLocation(const string & attribute) const {
		Map::const_iterator itr = attributes.find(attribute);
        if (attributes.end() != itr) {
            return itr->second;
        }
        return -1;
    }

    void uniformMat4(const string & uniform, const glm::mat4 & mat) const {
        glUniformMatrix4fv(getUniformLocation(uniform), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void uniform4f(const string & uniform, float a, float b, float c, float d) const{
        glUniform4f(getUniformLocation(uniform), a, b, c, d);
    }

    void uniform4f(const string & uniform, const float * fv) const {
        uniform4f(uniform, fv[0], fv[1], fv[2], fv[3]);
    }

    void uniform2f(const string & uniform, float a, float b) const {
        glUniform2f(getUniformLocation(uniform), a, b);
    }

    void uniform2f(const string & uniform, const glm::vec2 & vec) const {
        uniform2f(uniform, vec.x, vec.y);
    }
};

void checkGlError() {
    GLenum error = glGetError();
    if (error != 0) {
        cerr << error << endl;
    }
    assert(error == 0);
}

// We create a small class to handle the framework of creating the window,
// responding to user input, and executing the main loop
class glfwApp {
    GLFWwindow * window;
public:
    glfwApp() : window(	) {
        // Initialize the GLFW system for creating and positioning windows
        if( !glfwInit() ) {
            cerr << "Failed to initialize GLFW" << endl;
            exit( EXIT_FAILURE );
        }
        glfwSetErrorCallback(glfwErrorCallback);
    }

    virtual void createWindow(int w, int h, int x = 0, int y = 0) {
        glfwWindowHint(GLFW_DEPTH_BITS, 16);
        window = glfwCreateWindow(w, h, "glfw", NULL, NULL);
        assert(window != 0);
        glfwSetWindowUserPointer(window, this);
        glfwSetWindowPos(window, x, y);
        glfwSetKeyCallback(window, glfwKeyCallback);
        glfwMakeContextCurrent(window);

		// Initialize the OpenGL 3.x bindings
#ifdef WIN32
		if (0 != gl3wInit()) {
			cerr << "Failed to initialize GLEW" << endl;
			exit( EXIT_FAILURE );
		}
#endif

		checkGlError();
    }

    virtual ~glfwApp() {
        glfwTerminate();
    }


    virtual int run() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            update();
            draw();
            glfwSwapBuffers(window);
        }
        return 0;
    }

    virtual void onKey(int key, int scancode, int action, int mods) = 0;
    virtual void draw() = 0;
    virtual void update() = 0;

    static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        glfwApp * instance = (glfwApp *)glfwGetWindowUserPointer(window);
        instance->onKey(key, scancode, action, mods);
    }

    static void glfwErrorCallback(int error, const char* description) {
        cerr << description << endl;
        exit( EXIT_FAILURE );
    }
};


const StereoEye EYES[2] = { StereoEye_Left, StereoEye_Right };

class Example00 : public glfwApp {
protected:
    enum Mode {
        MONO, STEREO, STEREO_DISTORT
    };

    glm::mat4 projection;
    glm::mat4 modelview;
    Ptr<SensorDevice> ovrSensor;
    SensorFusion sensorFusion;
    StereoConfig stereoConfig;

	// Provides the resolution and location of the Rift
	HMDInfo hmdInfo;
	// Calculated width and height of the per-eye rendering area used
    int eyeWidth, eyeHeight;
	// Calculated width and height of the frame buffer object used to contain
	// intermediate results for the multipass render
	int fboWidth, fboHeight;

	Mode renderMode;
    bool useTracker;
    long elapsed;

    GLuint cubeVertexBuffer;
    GLuint cubeIndexBuffer;
    GLuint cubeWireIndexBuffer;

    GLuint quadVertexBuffer;
    GLuint quadIndexBuffer;

    GLprogram renderProgram;
    GLprogram textureProgram;
    GLprogram distortProgram;

    GLuint frameBuffer;
    GLuint frameBufferTexture;
    GLuint depthBuffer;

public:

    Example00() :  renderMode(MONO), useTracker(false), elapsed(0),
		cubeVertexBuffer(0), cubeIndexBuffer(0), cubeWireIndexBuffer(0), quadVertexBuffer(0), quadIndexBuffer(0),
		frameBuffer(0), frameBufferTexture(0), depthBuffer(0)
		{

		// do the master initialization for the Oculus VR SDK
		OVR::System::Init();

		sensorFusion.SetGravityEnabled(false);
		sensorFusion.SetPredictionEnabled(false);
		sensorFusion.SetYawCorrectionEnabled(false);

        hmdInfo.HResolution = 1280;
        hmdInfo.VResolution = 800;
        hmdInfo.HScreenSize = 0.149759993f;
        hmdInfo.VScreenSize = 0.0935999975f;
        hmdInfo.VScreenCenter = 0.0467999987f;
        hmdInfo.EyeToScreenDistance	= 0.0410000011f;
        hmdInfo.LensSeparationDistance = 0.0635000020f;
        hmdInfo.InterpupillaryDistance = 0.0640000030f;
        hmdInfo.DistortionK[0] = 1.00000000f;
        hmdInfo.DistortionK[1] = 0.219999999f;
        hmdInfo.DistortionK[2] = 0.239999995f;
        hmdInfo.DistortionK[3] = 0.000000000f;
        hmdInfo.ChromaAbCorrection[0] = 0.995999992f;
        hmdInfo.ChromaAbCorrection[1] = -0.00400000019f;
        hmdInfo.ChromaAbCorrection[2] = 1.01400006f;
        hmdInfo.ChromaAbCorrection[3] = 0.000000000f;
        hmdInfo.DesktopX = 0;
        hmdInfo.DesktopY = 0;

		///////////////////////////////////////////////////////////////////////////
        // Initialize Oculus VR SDK and hardware
        Ptr<DeviceManager> ovrManager = *DeviceManager::Create();
        if (ovrManager) {
            ovrSensor = *ovrManager->EnumerateDevices<SensorDevice>().CreateDevice();
            if (ovrSensor) {
                useTracker = true;
                sensorFusion.AttachToSensor(ovrSensor);
            }
            Ptr<HMDDevice> ovrHmd = *ovrManager->EnumerateDevices<HMDDevice>().CreateDevice();
            if (ovrHmd) {
                ovrHmd->GetDeviceInfo(&hmdInfo);
            }
            // The HMDInfo structure contains everything we need for now, so no
            // need to keep the device handle around
            ovrHmd.Clear();
        }
        // The device manager is reference counted and will be released automatically
        // when our sensorObject is destroyed.
        ovrManager.Clear();
        stereoConfig.SetHMDInfo(hmdInfo);

        ///////////////////////////////////////////////////////////////////////////
        // Create OpenGL window context
        createWindow(hmdInfo.HResolution, hmdInfo.VResolution, hmdInfo.DesktopX, hmdInfo.DesktopY);

        ///////////////////////////////////////////////////////////////////////////
        // Init OpenGL

        // Enable the zbuffer test
        glEnable(GL_DEPTH_TEST);
        glLineWidth(2.0f);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        checkGlError();

        glGenBuffers(1, &cubeVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER,
                sizeof(GLfloat) * VERT_COUNT * VERTICES_PER_TRIANGLE, CUBE_VERTEX_DATA, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &cubeIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                sizeof(GLuint) * FACE_COUNT * TRIANGLES_PER_FACE * VERTICES_PER_TRIANGLE,
                CUBE_INDICES, GL_STATIC_DRAW);

        glGenBuffers(1, &cubeWireIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeWireIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                sizeof(GLuint) * EDGE_COUNT * VERTICES_PER_EDGE,
                CUBE_WIRE_INDICES, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glGenBuffers(1, &quadVertexBuffer);
        glGenBuffers(1, &quadIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 6, QUAD_INDICES, GL_STATIC_DRAW);


        eyeWidth = hmdInfo.HResolution / 2;
        eyeHeight = hmdInfo.VResolution;
        fboWidth = eyeWidth * FRAMEBUFFER_OBJECT_SCALE;
        fboHeight = eyeHeight * FRAMEBUFFER_OBJECT_SCALE;

        glGenFramebuffers(1, &frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

        glGenTextures(1, &frameBufferTexture);
        glBindTexture(GL_TEXTURE_2D, frameBufferTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Allocate space for the texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fboWidth, fboHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBufferTexture, 0);
        glGenRenderbuffers(1, &depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, fboWidth, fboHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
        glEnable(GL_TEXTURE_2D);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Create the rendering shaders
        renderProgram.open("Simple");
        textureProgram.open("Texture");
        distortProgram.open("Distort");

        checkGlError();
        modelview = glm::lookAt(CAMERA, ORIGIN, UP);
        projection = glm::perspective(60.0f, (float)hmdInfo.HResolution / (float)hmdInfo.VResolution, 0.1f, 100.f);
	}

	virtual ~Example00() {
		OVR::System::Destroy();
	}

    virtual void onKey(int key, int scancode, int action, int mods) {
        if (GLFW_PRESS != action) {
            return;
        }
        switch (key) {
        case GLFW_KEY_R:
			sensorFusion.Reset();
			break;

        case GLFW_KEY_T:
            useTracker = !useTracker;
            break;

        case GLFW_KEY_P:
            renderMode = static_cast<Mode>((renderMode + 1) % 3);
            if (renderMode == MONO) {
                projection = glm::perspective(60.0f,
                        (float)hmdInfo.HResolution / (float)hmdInfo.VResolution, 0.1f, 100.f);
            } else if (renderMode == STEREO) {
                projection = glm::perspective(60.0f,
                        (float)hmdInfo.HResolution / 2.0f / (float)hmdInfo.VResolution, 0.1f, 100.f);
            } else if (renderMode == STEREO_DISTORT) {
                projection = glm::perspective(stereoConfig.GetYFOVDegrees(),
                        (float)hmdInfo.HResolution / 2.0f / (float)hmdInfo.VResolution, 0.1f, 100.f);
            }
            break;
        }
    }

    virtual void update() {
        long now = millis();
        if (useTracker) {
			// For some reason building the quaternion directly from the OVR
			// x,y,z,w values does not work.  So instead we convert it into
			// euler angles and construct our glm::quaternion from those

			// Fetch the pitch roll and yaw out of the sensorFusion device
			glm::vec3 eulerAngles;
			sensorFusion.GetOrientation().GetEulerAngles<Axis_X, Axis_Y, Axis_Z, Rotate_CW, Handed_R>(
				&eulerAngles.x, &eulerAngles.y, &eulerAngles.z);

			// Not convert it into a GLM quaternion.
			glm::quat orientation = glm::quat(eulerAngles);

			// Most applications want take a basic camera postion and apply the
			// orientation transform to it in this way:
			// modelview = glm::mat4_cast(orientation) * glm::lookAt(CAMERA, ORIGIN, UP);

			// However for this demonstration we want the cube to remain
			// centered in the viewport, and orbit our view around it.  This
			// serves two purposes.
			//
			// First, it's not possible to see a blank screen in the event
			// the HMD is oriented to point away from the origin of the scene.
			//
			// Second, a scene that has no points of reference other than a
			// single small object can be disorienting, leaving the user
			// feeling lost in a void.  Having a fixed object in the center
			// of the screen that you appear to be moving around should
			// provide less immersion, which in this instance is better
			modelview = glm::lookAt(CAMERA, ORIGIN, UP) * glm::mat4_cast(orientation);
        } else {
			// In the absence of head tracker information, we want to slowly
			// rotate the cube so that the animation of the scene is apparent
            static const float Y_ROTATION_RATE = 0.01f;
            static const float Z_ROTATION_RATE = 0.05f;
            modelview = glm::lookAt(CAMERA, ORIGIN, UP);
            modelview = glm::rotate(modelview, elapsed * Y_ROTATION_RATE, Y_AXIS);
            modelview = glm::rotate(modelview, elapsed * Z_ROTATION_RATE, Z_AXIS);
        }
        elapsed = now;
    }

    virtual void draw() {
        if (renderMode == MONO) {
            // If we're not working stereo, we're just going to render the
            // scene once, from a single position, directly to the back buffer
            glViewport(0, 0, hmdInfo.HResolution, hmdInfo.VResolution);
            renderScene(projection, modelview);
        } else {
            // If we get here, we're rendering in stereo, so we have to render our output twice

            // We have to explicitly clear the screen here.  the Clear command doesn't object the viewport
            // and the clear command inside renderScene will only target the active framebuffer object.
            glClearColor(0, 1, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            for (int i = 0; i < 2; ++i) {
                StereoEye eye = EYES[i];
                glBindTexture(GL_TEXTURE_2D, 0);
                glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
                glViewport(0, 0, fboWidth, fboHeight);

                // Compute the modelview and projection matrices for the rendered scene based on the eye and
                // whether or not we're doing side by side or rift rendering
                glm::vec3 eyeProjectionOffset(-stereoConfig.GetProjectionCenterOffset() / 2.0f, 0, 0);
                glm::vec3 eyeModelviewOffset = glm::vec3(-stereoConfig.GetIPD() / 2.0f, 0, 0);
                if (eye == StereoEye_Left) {
                    eyeModelviewOffset *= -1;
                    eyeProjectionOffset *= -1;
                }

                glm::mat4 eyeModelview = glm::translate(glm::mat4(), eyeModelviewOffset) * modelview;
                glm::mat4 eyeProjection = projection;
                if (renderMode == STEREO_DISTORT) {
                    eyeProjection = glm::translate(eyeProjection, eyeProjectionOffset);
                }
                renderScene(eyeProjection, eyeModelview);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                // Setup the viewport for the eye to which we're rendering
                glViewport(1 + (eye == StereoEye_Left ? 0 : eyeWidth), 1, eyeWidth - 2, eyeHeight - 2);

                GLprogram & program = (renderMode == STEREO_DISTORT) ? distortProgram : textureProgram;
                program.use();
                GLint positionLocation = program.getAttributeLocation("Position");
                assert(positionLocation > -1);
                GLint texCoordLocation = program.getAttributeLocation("TexCoord");
                assert(texCoordLocation > -1);

                float texL = 0, texR = 1, texT = 1, texB = 0;
                if (renderMode == STEREO_DISTORT) {
					// Pysical width of the viewport
					static float eyeScreenWidth = hmdInfo.HScreenSize / 2.0f;
					// The viewport goes from -1,1.  We want to get the offset
					// of the lens from the center of the viewport, so we only
					// want to look at the distance from 0, 1, so we divide in
					// half again
					static float halfEyeScreenWidth = eyeScreenWidth / 2.0f;

					// The distance from the center of the display panel (NOT
					// the center of the viewport) to the lens axis
					static float lensDistanceFromScreenCenter = hmdInfo.LensSeparationDistance / 2.0f;

					// Now we we want to turn the measurement from
					// millimeters into the range 0, 1
					static float lensDistanceFromViewportEdge = lensDistanceFromScreenCenter / halfEyeScreenWidth;

					// Finally, we want the distnace from the center, not the
					// distance from the edge, so subtract the value from 1
					static float lensOffset = 1.0f - lensDistanceFromViewportEdge;
					static glm::vec2 aspect(1.0, (float)eyeWidth / (float)eyeHeight);

                    glm::vec2 lensCenter(lensOffset, 0);

					// Texture coordinates need to be in lens-space for the
					// distort shader
                    texL = -1 - lensOffset;
                    texR = 1 - lensOffset;
                    texT = 1 / aspect.y;
                    texB = -1 / aspect.y;
					// Flip the values for the right eye
                    if (eye != StereoEye_Left) {
                        swap(texL, texR);
                        texL *= -1;
                        texR *= -1;
                        lensCenter *= -1;
                    }

					static glm::vec2 distortionScale(1.0f / stereoConfig.GetDistortionScale(),
						1.0f / stereoConfig.GetDistortionScale());
                    program.uniform2f("LensCenter", lensCenter);
                    program.uniform2f("Aspect", aspect);
					program.uniform2f("DistortionScale", distortionScale);
                    program.uniform4f("K", hmdInfo.DistortionK);
                }

                const GLfloat quadVertices[] = {
                    -1, -1, texL, texB,
                     1, -1, texR, texB,
                     1,  1, texR, texT,
                    -1,  1, texL, texT,
                };


                glBindTexture(GL_TEXTURE_2D, frameBufferTexture);
                glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
                glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * 2 * 4, quadVertices, GL_DYNAMIC_DRAW);

                int stride = sizeof(GLfloat) * 2 * 2;
                glEnableVertexAttribArray(positionLocation);
                glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, stride, 0);
                glEnableVertexAttribArray(texCoordLocation);
                glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(sizeof(GLfloat) * 2));

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIndexBuffer);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (GLvoid*)0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

                glBindBuffer(GL_ARRAY_BUFFER, 0);
            } // for
        } // if
    }

    virtual void renderScene(const glm::mat4 & projection, const glm::mat4 & modelview) {
        // Clear the buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Configure the GL pipeline for rendering our geometry
        renderProgram.use();

        // Load the projection and modelview matrices into the program
        renderProgram.uniformMat4("Projection", projection);
        renderProgram.uniformMat4("ModelView", modelview);

        // Load up our cube geometry (vertices and indices)
        glBindBuffer(GL_ARRAY_BUFFER, cubeVertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIndexBuffer);

        // Bind the vertex data to the program
        GLint positionLocation = renderProgram.getAttributeLocation("Position");
        GLint colorLocation = renderProgram.getUniformLocation("Color");

        glEnableVertexAttribArray(positionLocation);
        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 12, (GLvoid*)0);

        // Draw the cube faces, two calls for each face in order to set the color and then draw the geometry
        for (uintptr_t i = 0; i < FACE_COUNT; ++i) {
            renderProgram.uniform4f("Color", CUBE_FACE_COLORS + (i * 4));
            glDrawElements(GL_TRIANGLES, TRIANGLES_PER_FACE * VERTICES_PER_TRIANGLE, GL_UNSIGNED_INT, (void*)(i * 6 * 4));
        }

        // Now scale the modelview matrix slightly, so we can draw the cube outline
        glm::mat4 scaledCamera = glm::scale(modelview, glm::vec3(1.01f));
        renderProgram.uniformMat4("ModelView", scaledCamera);

        // Drawing a white wireframe around the cube
        glUniform4f(colorLocation, 1, 1, 1, 1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeWireIndexBuffer);
        glDrawElements(GL_LINES, EDGE_COUNT * VERTICES_PER_EDGE, GL_UNSIGNED_INT, (void*)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);

        checkGlError();
    }
};


#ifdef WIN32
    int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
    int main(int argc, char ** argv) {

    // Window and Apple both support applications embedded resources.  For
    // Linux we're going to try to locate the shaders relative to the
    // executable
    #ifndef __APPLE__
        string executable(argv[0]);
        string::size_type sep = executable.rfind('/');
        if (sep != string::npos) {
            executableDirectory = executable.substr(0, sep) + "/Resources";
        }
    #endif

#endif

    return Example00().run();
}
