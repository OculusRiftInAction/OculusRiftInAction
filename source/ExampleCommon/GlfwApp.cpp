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
#include "GlfwApp.h"


namespace RiftExamples {

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    GlfwApp * instance = (GlfwApp *)glfwGetWindowUserPointer(window);
    instance->onKey(key, scancode, action, mods);
}

void glfwErrorCallback(int error, const char* description) {
    FAIL(description);
}

GlfwApp::GlfwApp() : window(0), width(0), height(0) {
    // Initialize the GLFW system for creating and positioning windows
    if( !glfwInit() ) {
        FAIL("Failed to initialize GLFW");
    }
    glfwSetErrorCallback(glfwErrorCallback);
}

void GlfwApp::createWindow(int w, int h, int x, int y) {
    width = w;
    height = h;
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    window = glfwCreateWindow(w, h, "glfw", NULL, NULL);
    assert(window != 0);
    if (x > INT_MIN && y > INT_MIN) {
        glfwSetWindowPos(window, x, y);
    }
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, glfwKeyCallback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize the OpenGL bindings
    if (0 != glewInit()) {
        FAIL("Failed to initialize GL3W");
    }
}

void GlfwApp::initGl() {
}

void GlfwApp::destroyWindow() {
    glfwSetKeyCallback(window, nullptr);
    glfwDestroyWindow(window);
}

GlfwApp::~GlfwApp() {
    glfwTerminate();
}


int GlfwApp::run() {
    if (!window) {
        return -1;
    }
    initGl();

    int framecount = 0;
    long start = Platform::elapsedMillis();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        update();
        draw();
        glfwSwapBuffers(window);
        long now = Platform::elapsedMillis();
        ++framecount;
        if ((now - start) >= 2000) {
            float elapsed = (now - start) / 1000.f;
            float fps = (float)framecount / elapsed;
            std::cout << "FPS: " << fps << std::endl;
            start = now;
            framecount = 0;
        }
    }
    return 0;
}

void GlfwApp::onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
        return;
    }

    switch (key) {
    case GLFW_KEY_ESCAPE:
        exit(0);
    }
}

void GlfwApp::draw() {
}

void GlfwApp::update() {
};

}
