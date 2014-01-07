#version 330

uniform bool Mirror;
uniform float LensOffset;
uniform float ViewportAspectRatio;

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord0;

out vec2 vTexCoord;

void main() {
  // set the texture coordinate, but put it into aspect corrected terms.
  // and move the origin to the center of the texture.  This will be
  // reversed in the fragment shader
  vTexCoord = TexCoord0;
  vTexCoord *= 2.0;
  vTexCoord -= 1.0;

  gl_Position = vec4(Position, 1);
  float eyeLensOffset = LensOffset;
  eyeLensOffset *= (Mirror ? -1 : 1);
  gl_Position.x += eyeLensOffset;
}
