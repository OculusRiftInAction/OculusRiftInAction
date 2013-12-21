#version 420

uniform float Aspect = 1.0;
uniform bool Mirror = false;
uniform vec2 LensCenter = vec2(0, 0);

layout(location = 0) in vec3 Position;

out vec2 vRiftTexCoord;

void main() {
    vRiftTexCoord = Position.xy;
    vRiftTexCoord.y /= Aspect;
    vRiftTexCoord -= ( LensCenter * (Mirror  ? -1.0 : 1.0));
    gl_Position = vec4(Position.xy, 0, 1);
}
