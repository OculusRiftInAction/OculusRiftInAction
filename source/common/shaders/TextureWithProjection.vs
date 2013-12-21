#version 330

uniform float ViewportAspectRatio = 1.0;

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord0;

out vec2 vTexCoord;

void main() {
    vTexCoord = TexCoord0;
    gl_Position = Position;
    gl_Position.y *= ViewportAspectRatio;
}
