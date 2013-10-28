#version 330
#extension GL_ARB_explicit_uniform_location : enable
layout(location = 0) uniform mat4 Projection;
layout(location = 1) uniform mat4 ModelView;

layout(location = 0) in vec3 Position;

void main() {
   gl_Position = Projection * ModelView * vec4(Position, 1);
}
