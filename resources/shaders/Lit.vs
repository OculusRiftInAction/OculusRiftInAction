#version 330

layout(location = 0) in vec3 Position;
layout(location = 2) in vec3 Normal;

uniform mat4 Projection;
uniform mat4 ModelView;

out vec3 vViewNormal;
out vec3 vViewPosition;

void main() {
    vec4 Position4 = vec4(Position, 1);
    vec4 Normal4 = vec4(Normal, 0);
    gl_Position = Projection * ModelView * Position4;


    // The normal in view space
    vViewNormal = vec3(ModelView * Normal4);

    // The position in view space
    vViewPosition = vec3(ModelView * Position4);
}

