#version 330

uniform mat4 Projection;
uniform mat4 ModelView;

layout(location = 0) in vec3 Position;
layout(location = 8) in vec2 TexCoord0;

out vec2 vTexCoord;

void main() {
    vTexCoord = TexCoord0;
    gl_Position = Projection * ModelView * vec4(Position, 1);
}
