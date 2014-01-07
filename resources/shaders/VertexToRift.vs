#version 330

uniform float Aspect = 1.0;
uniform float LensOffset = 0.0;

layout(location = 0) in vec4 Position;

out vec2 vRiftTexCoord;

void main() {
    vRiftTexCoord = Position.xy;
    vRiftTexCoord.y /= Aspect;
    vRiftTexCoord.x -= LensOffset;
    gl_Position = Position;
}
