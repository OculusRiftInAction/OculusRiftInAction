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
#include <glm/gtc/type_ptr.hpp>
#include "Types.h"

// Some defines to make calculations below more transparent
#define TRIANGLES_PER_FACE 2
#define VERTICES_PER_TRIANGLE 3
#define VERTICES_PER_EDGE 2
#define FLOATS_PER_VERTEX 3

#define CUBE_SIZE 1.0f
#define CUBE_P (CUBE_SIZE / 2.0f)
#define CUBE_N (-1.0f * CUBE_P)

// Cube geometry
#define CUBE_VERT_COUNT 8
#define CUBE_FACE_COUNT 6
#define CUBE_EDGE_COUNT 12

namespace RiftExamples {

using namespace GL;

// Adapted from OpenFrameworks
const glm::vec3 Colors::gray(1.0f / 2, 1.0f / 2, 1.0f / 2);
const glm::vec3 Colors::white(1.0f, 1.0f, 1.0f);
const glm::vec3 Colors::red(1.0f, 0, 0);
const glm::vec3 Colors::green(0, 1.0f, 0);
const glm::vec3 Colors::blue(0, 0, 1.0f);
const glm::vec3 Colors::cyan(0, 1.0f, 1.0f);
const glm::vec3 Colors::magenta(1.0f, 0, 1.0f);
const glm::vec3 Colors::yellow(1.0f, 1.0f, 0);
const glm::vec3 Colors::black(0, 0, 0);
const glm::vec3 Colors::aliceBlue(0.941176,0.972549,1);
const glm::vec3 Colors::antiqueWhite(0.980392,0.921569,0.843137);
const glm::vec3 Colors::aqua(0,1,1);
const glm::vec3 Colors::aquamarine(0.498039,1,0.831373);
const glm::vec3 Colors::azure(0.941176,1,1);
const glm::vec3 Colors::beige(0.960784,0.960784,0.862745);
const glm::vec3 Colors::bisque(1,0.894118,0.768627);
const glm::vec3 Colors::blanchedAlmond(1,0.921569,0.803922);
const glm::vec3 Colors::blueViolet(0.541176,0.168627,0.886275);
const glm::vec3 Colors::brown(0.647059,0.164706,0.164706);
const glm::vec3 Colors::burlyWood(0.870588,0.721569,0.529412);
const glm::vec3 Colors::cadetBlue(0.372549,0.619608,0.627451);
const glm::vec3 Colors::chartreuse(0.498039,1,0);
const glm::vec3 Colors::chocolate(0.823529,0.411765,0.117647);
const glm::vec3 Colors::coral(1,0.498039,0.313726);
const glm::vec3 Colors::cornflowerBlue(0.392157,0.584314,0.929412);
const glm::vec3 Colors::cornsilk(1,0.972549,0.862745);
const glm::vec3 Colors::crimson(0.862745,0.0784314,0.235294);
const glm::vec3 Colors::darkBlue(0,0,0.545098);
const glm::vec3 Colors::darkCyan(0,0.545098,0.545098);
const glm::vec3 Colors::darkGoldenRod(0.721569,0.52549,0.0431373);
const glm::vec3 Colors::darkGray(0.662745,0.662745,0.662745);
const glm::vec3 Colors::darkGrey(0.662745,0.662745,0.662745);
const glm::vec3 Colors::darkGreen(0,0.392157,0);
const glm::vec3 Colors::darkKhaki(0.741176,0.717647,0.419608);
const glm::vec3 Colors::darkMagenta(0.545098,0,0.545098);
const glm::vec3 Colors::darkOliveGreen(0.333333,0.419608,0.184314);
const glm::vec3 Colors::darkorange(1,0.54902,0);
const glm::vec3 Colors::darkOrchid(0.6,0.196078,0.8);
const glm::vec3 Colors::darkRed(0.545098,0,0);
const glm::vec3 Colors::darkSalmon(0.913725,0.588235,0.478431);
const glm::vec3 Colors::darkSeaGreen(0.560784,0.737255,0.560784);
const glm::vec3 Colors::darkSlateBlue(0.282353,0.239216,0.545098);
const glm::vec3 Colors::darkSlateGray(0.184314,0.309804,0.309804);
const glm::vec3 Colors::darkSlateGrey(0.184314,0.309804,0.309804);
const glm::vec3 Colors::darkTurquoise(0,0.807843,0.819608);
const glm::vec3 Colors::darkViolet(0.580392,0,0.827451);
const glm::vec3 Colors::deepPink(1,0.0784314,0.576471);
const glm::vec3 Colors::deepSkyBlue(0,0.74902,1);
const glm::vec3 Colors::dimGray(0.411765,0.411765,0.411765);
const glm::vec3 Colors::dimGrey(0.411765,0.411765,0.411765);
const glm::vec3 Colors::dodgerBlue(0.117647,0.564706,1);
const glm::vec3 Colors::fireBrick(0.698039,0.133333,0.133333);
const glm::vec3 Colors::floralWhite(1,0.980392,0.941176);
const glm::vec3 Colors::forestGreen(0.133333,0.545098,0.133333);
const glm::vec3 Colors::fuchsia(1,0,1);
const glm::vec3 Colors::gainsboro(0.862745,0.862745,0.862745);
const glm::vec3 Colors::ghostWhite(0.972549,0.972549,1);
const glm::vec3 Colors::gold(1,0.843137,0);
const glm::vec3 Colors::goldenRod(0.854902,0.647059,0.12549);
const glm::vec3 Colors::grey(0.501961,0.501961,0.501961);
const glm::vec3 Colors::greenYellow(0.678431,1,0.184314);
const glm::vec3 Colors::honeyDew(0.941176,1,0.941176);
const glm::vec3 Colors::hotPink(1,0.411765,0.705882);
const glm::vec3 Colors::indianRed (0.803922,0.360784,0.360784);
const glm::vec3 Colors::indigo (0.294118,0,0.509804);
const glm::vec3 Colors::ivory(1,1,0.941176);
const glm::vec3 Colors::khaki(0.941176,0.901961,0.54902);
const glm::vec3 Colors::lavender(0.901961,0.901961,0.980392);
const glm::vec3 Colors::lavenderBlush(1,0.941176,0.960784);
const glm::vec3 Colors::lawnGreen(0.486275,0.988235,0);
const glm::vec3 Colors::lemonChiffon(1,0.980392,0.803922);
const glm::vec3 Colors::lightBlue(0.678431,0.847059,0.901961);
const glm::vec3 Colors::lightCoral(0.941176,0.501961,0.501961);
const glm::vec3 Colors::lightCyan(0.878431,1,1);
const glm::vec3 Colors::lightGoldenRodYellow(0.980392,0.980392,0.823529);
const glm::vec3 Colors::lightGray(0.827451,0.827451,0.827451);
const glm::vec3 Colors::lightGrey(0.827451,0.827451,0.827451);
const glm::vec3 Colors::lightGreen(0.564706,0.933333,0.564706);
const glm::vec3 Colors::lightPink(1,0.713726,0.756863);
const glm::vec3 Colors::lightSalmon(1,0.627451,0.478431);
const glm::vec3 Colors::lightSeaGreen(0.12549,0.698039,0.666667);
const glm::vec3 Colors::lightSkyBlue(0.529412,0.807843,0.980392);
const glm::vec3 Colors::lightSlateGray(0.466667,0.533333,0.6);
const glm::vec3 Colors::lightSlateGrey(0.466667,0.533333,0.6);
const glm::vec3 Colors::lightSteelBlue(0.690196,0.768627,0.870588);
const glm::vec3 Colors::lightYellow(1,1,0.878431);
const glm::vec3 Colors::lime(0,1,0);
const glm::vec3 Colors::limeGreen(0.196078,0.803922,0.196078);
const glm::vec3 Colors::linen(0.980392,0.941176,0.901961);
const glm::vec3 Colors::maroon(0.501961,0,0);
const glm::vec3 Colors::mediumAquaMarine(0.4,0.803922,0.666667);
const glm::vec3 Colors::mediumBlue(0,0,0.803922);
const glm::vec3 Colors::mediumOrchid(0.729412,0.333333,0.827451);
const glm::vec3 Colors::mediumPurple(0.576471,0.439216,0.858824);
const glm::vec3 Colors::mediumSeaGreen(0.235294,0.701961,0.443137);
const glm::vec3 Colors::mediumSlateBlue(0.482353,0.407843,0.933333);
const glm::vec3 Colors::mediumSpringGreen(0,0.980392,0.603922);
const glm::vec3 Colors::mediumTurquoise(0.282353,0.819608,0.8);
const glm::vec3 Colors::mediumVioletRed(0.780392,0.0823529,0.521569);
const glm::vec3 Colors::midnightBlue(0.0980392,0.0980392,0.439216);
const glm::vec3 Colors::mintCream(0.960784,1,0.980392);
const glm::vec3 Colors::mistyRose(1,0.894118,0.882353);
const glm::vec3 Colors::moccasin(1,0.894118,0.709804);
const glm::vec3 Colors::navajoWhite(1,0.870588,0.678431);
const glm::vec3 Colors::navy(0,0,0.501961);
const glm::vec3 Colors::oldLace(0.992157,0.960784,0.901961);
const glm::vec3 Colors::olive(0.501961,0.501961,0);
const glm::vec3 Colors::oliveDrab(0.419608,0.556863,0.137255);
const glm::vec3 Colors::orange(1,0.647059,0);
const glm::vec3 Colors::orangeRed(1,0.270588,0);
const glm::vec3 Colors::orchid(0.854902,0.439216,0.839216);
const glm::vec3 Colors::paleGoldenRod(0.933333,0.909804,0.666667);
const glm::vec3 Colors::paleGreen(0.596078,0.984314,0.596078);
const glm::vec3 Colors::paleTurquoise(0.686275,0.933333,0.933333);
const glm::vec3 Colors::paleVioletRed(0.858824,0.439216,0.576471);
const glm::vec3 Colors::papayaWhip(1,0.937255,0.835294);
const glm::vec3 Colors::peachPuff(1,0.854902,0.72549);
const glm::vec3 Colors::peru(0.803922,0.521569,0.247059);
const glm::vec3 Colors::pink(1,0.752941,0.796078);
const glm::vec3 Colors::plum(0.866667,0.627451,0.866667);
const glm::vec3 Colors::powderBlue(0.690196,0.878431,0.901961);
const glm::vec3 Colors::purple(0.501961,0,0.501961);
const glm::vec3 Colors::rosyBrown(0.737255,0.560784,0.560784);
const glm::vec3 Colors::royalBlue(0.254902,0.411765,0.882353);
const glm::vec3 Colors::saddleBrown(0.545098,0.270588,0.0745098);
const glm::vec3 Colors::salmon(0.980392,0.501961,0.447059);
const glm::vec3 Colors::sandyBrown(0.956863,0.643137,0.376471);
const glm::vec3 Colors::seaGreen(0.180392,0.545098,0.341176);
const glm::vec3 Colors::seaShell(1,0.960784,0.933333);
const glm::vec3 Colors::sienna(0.627451,0.321569,0.176471);
const glm::vec3 Colors::silver(0.752941,0.752941,0.752941);
const glm::vec3 Colors::skyBlue(0.529412,0.807843,0.921569);
const glm::vec3 Colors::slateBlue(0.415686,0.352941,0.803922);
const glm::vec3 Colors::slateGray(0.439216,0.501961,0.564706);
const glm::vec3 Colors::slateGrey(0.439216,0.501961,0.564706);
const glm::vec3 Colors::snow(1,0.980392,0.980392);
const glm::vec3 Colors::springGreen(0,1,0.498039);
const glm::vec3 Colors::steelBlue(0.27451,0.509804,0.705882);
const glm::vec3 Colors::blueSteel(0.27451,0.509804,0.705882);
const glm::vec3 Colors::tan(0.823529,0.705882,0.54902);
const glm::vec3 Colors::teal(0,0.501961,0.501961);
const glm::vec3 Colors::thistle(0.847059,0.74902,0.847059);
const glm::vec3 Colors::tomato(1,0.388235,0.278431);
const glm::vec3 Colors::turquoise(0.25098,0.878431,0.815686);
const glm::vec3 Colors::violet(0.933333,0.509804,0.933333);
const glm::vec3 Colors::wheat(0.960784,0.870588,0.701961);
const glm::vec3 Colors::whiteSmoke(0.960784,0.960784,0.960784);
const glm::vec3 Colors::yellowGreen(0.603922,0.803922,0.196078);


// Vertices for a unit cube centered at the origin
const GLfloat CUBE_VERTEX_DATA[] = {
    CUBE_N, CUBE_N, CUBE_N, 0, // Vertex 0 position
    CUBE_P, CUBE_N, CUBE_N, 0, // Vertex 1 position
    CUBE_P, CUBE_P, CUBE_N, 0, // Vertex 2 position
    CUBE_N, CUBE_P, CUBE_N, 0, // Vertex 3 position
    CUBE_N, CUBE_N, CUBE_P, 0, // Vertex 4 position
    CUBE_P, CUBE_N, CUBE_P, 0, // Vertex 5 position
    CUBE_P, CUBE_P, CUBE_P, 0, // Vertex 6 position
    CUBE_N, CUBE_P, CUBE_P, 0, // Vertex 7 position
};

const glm::vec3 CUBE_FACE_COLORS[] = {
        Colors::red,
        Colors::green,
        Colors::blue,
        Colors::yellow,
        Colors::cyan,
        Colors::magenta,
};

// 6 sides * 2 triangles * 3 vertices
const unsigned int CUBE_INDICES[CUBE_FACE_COUNT * TRIANGLES_PER_FACE * VERTICES_PER_TRIANGLE ] = {
   0, 4, 5, 0, 5, 1, // Face 0
   1, 5, 6, 1, 6, 2, // Face 1
   2, 6, 7, 2, 7, 3, // Face 2
   3, 7, 4, 3, 4, 0, // Face 3
   4, 7, 6, 4, 6, 5, // Face 4
   3, 0, 1, 3, 1, 2  // Face 5
};

//
const unsigned int CUBE_WIRE_INDICES[CUBE_EDGE_COUNT * VERTICES_PER_EDGE ] = {
   0, 1, 1, 2, 2, 3, 3, 0, // square
   4, 5, 5, 6, 6, 7, 7, 4, // facing square
   0, 4, 1, 5, 2, 6, 3, 7, // transverse lines
};

const GLuint QUAD_INDICES[] = {
   0, 1, 2, // Triangle 1
   2, 0, 3, // Triangle 2
};

const GLfloat QUAD_VERTICES[] = {
    -1, -1, 0, 0,
     1, -1, 0, 0,
     1,  1, 0, 0,
    -1,  1, 0, 0,
};


const glm::vec3 GlUtils::X_AXIS = glm::vec3(1.0f, 0.0f, 0.0f);
const glm::vec3 GlUtils::Y_AXIS = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 GlUtils::Z_AXIS = glm::vec3(0.0f, 0.0f, 1.0f);
const glm::vec3 GlUtils::ORIGIN = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 GlUtils::UP = glm::vec3(0.0f, 1.0f, 0.0f);


VertexBufferPtr GlUtils::getQuadVertices() {
    VertexBufferPtr result(new VertexBuffer());
    // Create the buffers for the texture quad we will draw
    result->init();
    result->bind();
    (*result) << GL::makeArrayLoader(QUAD_VERTICES);
    result->unbind();
    GL_CHECK_ERROR;
    return result;
}

IndexBufferPtr GlUtils::getQuadIndices() {
    IndexBufferPtr result(new IndexBuffer());
    result->init();
    result->bind();
    (*result) << GL::makeArrayLoader(QUAD_INDICES);
    result->unbind();
    GL_CHECK_ERROR;
    return result;
}

VertexBufferPtr GlUtils::getCubeVertices() {
    VertexBufferPtr result(new VertexBuffer());
    // Create the buffers for the texture quad we will draw
    result->init();
    result->bind();
    (*result) << GL::makeArrayLoader(CUBE_VERTEX_DATA);
    result->unbind();
    GL_CHECK_ERROR;
    return result;
}

IndexBufferPtr GlUtils::getCubeIndices() {
    IndexBufferPtr result(new IndexBuffer());
    result->init();
    result->bind();
    (*result) << GL::makeArrayLoader(CUBE_INDICES);
    result->unbind();
    GL_CHECK_ERROR;
    return result;
}

IndexBufferPtr GlUtils::getCubeWireIndices() {
    IndexBufferPtr result(new IndexBuffer());
    result->init();
    result->bind();
    (*result) << GL::makeArrayLoader(CUBE_WIRE_INDICES);
    result->unbind();
    GL_CHECK_ERROR;
    return result;
}

void GlUtils::tumble(const glm::vec3 & camera)  {
    static const float Y_ROTATION_RATE = 0.01f;
    static const float Z_ROTATION_RATE = 0.05f;
           glm::mat4 & modelview = GL::Stacks::modelview().top();
           modelview = glm::lookAt(camera, ORIGIN, UP);
           modelview = glm::rotate(modelview,
                   Platform::elapsedMillis() * Y_ROTATION_RATE,
                   GlUtils::Y_AXIS);
           modelview = glm::rotate(modelview,
                   Platform::elapsedMillis() * Z_ROTATION_RATE,
                   GlUtils::Z_AXIS);
}


//    void ArtificialHorizon::customDraw() {
//        ofPushMatrix();
//            ofRotateX(90);
//            ofSetColor(255, 100, 0);
//            hemi.draw();
//            ofSetColor(20);
//            ofTranslate(0, 0, 1.02);
//            ofCircle(0, 0, 0.25);
//        ofPopMatrix();
//
//        ofPushMatrix();
//            ofRotateX(-90);
//            ofSetColor(100, 100, 255);
//            hemi.draw();
//            ofSetColor(20);
//            ofTranslate(0, 0, 1.02);
//            ofCircle(0, 0, 0.25);
//        ofPopMatrix();
//
//        ofSetColor(20);
//
//        ofPushMatrix();
//            ofTranslate(0, 0, 1.02);
//            ofRect(-0.50, -0.02, 1.0, 0.04);
//        ofPopMatrix();

void GlUtils::renderMesh(Resource meshName, Resource shaderName) {
    ProgramPtr program = Platform::getProgram(shaderName);
    program->use();
    Stacks::lights().apply(program);
    Stacks::projection().apply(program);
    Stacks::modelview().apply(program);

    Geometry & geometry = *Platform::getGeometry(meshName);
    geometry.bindVertexArray();
    program->checkConfigured();
    geometry.draw();

    VertexArray::unbind();
    Program::clear();
}

void GlUtils::renderBunny(Resource shaderName) {
    ProgramPtr program = Platform::getProgram(shaderName);
    program->use();
    program->setUniform4f("Color", glm::vec4(1));
    renderMesh(Resource::MESHES_BUNNY2_CTM, shaderName);
}

void GlUtils::renderArtificialHorizon(Resource shaderName) {
    Program & renderProgram = *Platform::getProgram(shaderName);
    // Configure the GL pipeline for rendering our geometry
    renderProgram.use();
    Stacks::lights().apply(renderProgram);
    Stacks::projection().apply(renderProgram);

    Geometry & geometry = *Platform::getGeometry(Resource::MESHES_HEMI_CTM);
    geometry.bindVertexArray();

    MatrixStack & mv = Stacks::modelview();

    renderProgram.setUniform4f("Color", glm::vec4(0.4, 0.4, 1.0, 1));
    mv.push().rotate(90, GlUtils::X_AXIS).apply(renderProgram);
    geometry.draw();
    mv.pop();

    renderProgram.setUniform4f("Color", glm::vec4(0.8, 0.4, 0.2, 1));
    mv.push().rotate(-90, GlUtils::X_AXIS).apply(renderProgram);
    geometry.draw();
    mv.pop();

    VertexArray::unbind();
    Program::clear();
}

void GlUtils::drawQuad(const glm::vec2 & min, const glm::vec2 & max) {
    glBegin(GL_QUADS);
        glVertex2f(min.x, min.y);
        glVertex2f(max.x, min.y);
        glVertex2f(max.x, max.y);
        glVertex2f(min.x, max.y);
    glEnd();
}

void GlUtils::drawColorCube() {
    // These hold the vertices, indices and the binding between the
    // Shader variable names and the values loaded into video memory
    static GeometryPtr cubeGeometry(new Geometry(
            getCubeVertices(), getCubeIndices(),
            CUBE_FACE_COUNT * TRIANGLES_PER_FACE, 0));
    static GeometryPtr cubeWireGeometry(new Geometry(
            getCubeVertices(), getCubeWireIndices(),
            CUBE_EDGE_COUNT * VERTICES_PER_EDGE, 0));

    GL::Program & renderProgram = *Platform::getProgram(Resource::SHADERS_SIMPLE_VS);
    renderProgram.use();

    // Load the projection and modelview matrices into the program
    Stacks::projection().apply(renderProgram);
    Stacks::modelview().apply(renderProgram);

    // Draw the cube faces, two calls for each face in order to set the color and then draw the geometry
    cubeGeometry->bindVertexArray();
    for (uintptr_t i = 0; i < CUBE_FACE_COUNT; ++i) {
        renderProgram.setUniform4f("Color", glm::vec4(CUBE_FACE_COLORS[i], 1));
        glDrawElements(GL_TRIANGLES, TRIANGLES_PER_FACE * VERTICES_PER_TRIANGLE, GL_UNSIGNED_INT, (void*)(i * 6 * 4));
    }
    VertexArray::unbind();

    // Now scale the modelview matrix slightly, so we can draw the cube outline
    // The apply method copies the top value into the shader, so we don't need to
    // keep the stack pushed while we render
    Stacks::modelview().push().scale(1.01f).apply(renderProgram).pop();
//    glLineWidth(1.0);
    // Drawing a white wireframe around the cube
    renderProgram.setUniform4f("Color", glm::vec4(Colors::white, 1));
    cubeWireGeometry->bindVertexArray();
    glDrawElements(GL_LINES, CUBE_EDGE_COUNT * VERTICES_PER_EDGE, GL_UNSIGNED_INT, (void*)0);

    VertexArray::unbind();
    glUseProgram(0);
}


void GlUtils::drawAngleTicks() {
    // Only necessary if you're using the fixed function pipeline
    // Fix the modelview at exactly 1 unit away from the origin, no rotation
    GL::Stacks::modelview().push(glm::mat4(1)).translate(glm::vec3(0, 0, -1));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(GL::Stacks::modelview().top()));
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(GL::Stacks::projection().top()));

    float offsets[] = { //
       (float)tan( PI / 6.0f ), // 30 degrees
       (float)tan( PI / 4.0f ), // 45 degrees
       (float)tan( PI / 3.0f) // 60 degrees
    };
    // 43.9 degrees puts tick on the inner edge of the screen
//           tan( PI / 4.22f ), // 42.6 degrees is the effective fov for wide screen
//                             // 43.9 degrees puts tick on the inner edge of the screen


    glLineWidth(4.0);
    glBegin(GL_LINES);
    glColor3f(1,1,1);
    GL::vertex(glm::vec3(-2, 0, 0));
    GL::vertex(glm::vec3(2, 0, 0));
    GL::vertex(glm::vec3(0, -2, 0));
    GL::vertex(glm::vec3(0, 2, 0));

    // By keeping the camera locked at 1 unit away from the origin, all our
    // distances can be computer as tan(angle)
    for (float offset : offsets) {
        glColor3f(0,1,1);
        GL::vertex(glm::vec3(offset, -0.05, 0));
        GL::vertex(glm::vec3(offset, 0.05, 0));
        GL::vertex(glm::vec3(-offset, -0.05, 0));
        GL::vertex(glm::vec3(-offset, 0.05, 0));
    }
    for (float offset : offsets) {
        glColor3f(0,1,1);
        GL::vertex(glm::vec3(-0.05, offset, 0));
        GL::vertex(glm::vec3(0.05, offset, 0));
        GL::vertex(glm::vec3( -0.05, -offset, 0));
        GL::vertex(glm::vec3( 0.05, -offset, 0));
    }
    glEnd();

    GL::Stacks::modelview().pop();
}

void GlUtils::draw3dGrid() {

    glLineWidth(1.0);
    glBegin(GL_LINES);
        GL::color(glm::vec3(0.25));
        for (int i = 0; i < 5; ++i) {
            float offset = ((float)i * 0.2f) + 0.2f;
            glm::vec3 zOffset(0, 0, offset);
            glm::vec3 xOffset(offset, 0, 0);
            GL::vertex(-GlUtils::X_AXIS + zOffset);
            GL::vertex(GlUtils::X_AXIS + zOffset);
            GL::vertex(-GlUtils::X_AXIS - zOffset);
            GL::vertex(GlUtils::X_AXIS - zOffset);
            GL::vertex(-GlUtils::Z_AXIS + xOffset);
            GL::vertex(GlUtils::Z_AXIS + xOffset);
            GL::vertex(-GlUtils::Z_AXIS - xOffset);
            GL::vertex(GlUtils::Z_AXIS - xOffset);
        }
        GL::vertex(glm::vec3(0));
        GL::vertex(-GlUtils::X_AXIS);
        GL::vertex(glm::vec3(0));
        GL::vertex(-GlUtils::Z_AXIS);

        GL::color(GlUtils::X_AXIS / 1.5f);
        GL::vertex(glm::vec3(0));
        GL::vertex(GlUtils::X_AXIS);
        GL::color(GlUtils::Y_AXIS / 1.5f);
        GL::vertex(glm::vec3(0));
        GL::vertex(GlUtils::Y_AXIS);
        GL::color(GlUtils::Z_AXIS / 1.5f);
        GL::vertex(glm::vec3(0));
        GL::vertex(GlUtils::Z_AXIS);
    glEnd();
}

using namespace Text;

void GlUtils::renderString(const Text::FontPtr & font, const std::wstring & str, glm::vec2 & cursor, float size) {

    GL::ProgramPtr program = Platform::getProgram(Resource::SHADERS_TEXT_VS, Resource::SHADERS_TEXT_FS);
    program->use();
    program->setUniform4f("Color", glm::vec4(1));
    program->setUniform1i("Font", 0);

    GL::Stacks::projection().apply(program);

    // Stores how far we've moved from the start of the string, in DTP units
    float advance = 0;
    float scale = Text::Font::DTP_TO_METERS * size / font->mFontSize;

    // Scale the curser from meters to font units
    glm::vec2 localCursor = cursor / scale;
    // And scale the modelview from into font units
    GL::MatrixStack & mv = GL::Stacks::modelview().push().scale(scale);
    font->mTexture->bind();
    font->mGeometry->bindVertexArray();
    std::for_each(str.begin(), str.end(), [&] (uint16_t id) {
        if (font->contains(id)) {
            // get metrics for this character to speed up measurements
            const Font::Metrics & m = font->getMetrics(id);
            // We create an offset vec2 to hold the local offset of this character
            // This includes compensating for the inverted Y axis of the font
            // coordinates
            glm::vec2 offset(advance + m.dx, 2 * m.dy - m.h);
            // Bind the new position
            mv.push().translate(offset + localCursor).apply(program).pop();
            // Render the item
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*) (m.indexOffset * sizeof(GLuint)));
            advance += m.d;// font->getAdvance(m, mFontSize);
        }
    });
    mv.pop();
    cursor.x += advance * scale;

    GL::VertexArray::unbind();
    GL::Texture::unbind();
    GL::Program::clear();
}


std::wstring toUtf16(const std::string & text) {
//    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide(text.begin(), text.end()); //= converter.from_bytes(narrow.c_str());
    return wide;
}



//!
void GlUtils::renderString(const Text::FontPtr & font, const std::string & str, glm::vec2 & cursor, float fontSize) {
    renderString(font, toUtf16(str), cursor, fontSize);
}

void GlUtils::renderString(const Text::FontPtr & font, const std::string & str, float size) {
    renderString(font, toUtf16(str), size);
}

void GlUtils::renderString(const Text::FontPtr & font, const std::wstring & str, float size) {
    glm::vec2 cursor;
    renderString(font, str, cursor, size);
}

rectf GlUtils::measure(const Text::FontPtr & font, const std::string &text, float fontSize) {
    return font->measure(toUtf16(text), fontSize);
}

//!
float GlUtils::measureWidth(const Text::FontPtr & font, const std::string &text, float fontSize, bool precise) {
    return font->measureWidth(toUtf16(text), fontSize, precise);
}


}
