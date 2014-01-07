#version 330

uniform float ViewportAspectRatio = 1.0;
uniform bool Mirror;
uniform float LensOffset;

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord0;


out vec2 vTexCoord;
out vec4 FragColor;

void main() {
  vTexCoord = TexCoord0;

  float eyeLensOffset = LensOffset;
  eyeLensOffset *= (Mirror ? -1.0 : 1.0);

  gl_Position = Position;
  gl_Position.y *= ViewportAspectRatio;
  gl_Position.x += eyeLensOffset;
}
