#version 420

layout(location = 0) in vec3 Position;

out vec2 vTexCoord;

void main() {
    vTexCoord = (Position.xy / 2.0) + 0.5;
    gl_Position = vec4(Position.xy, 0, 1);
}
