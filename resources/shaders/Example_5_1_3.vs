#version 330

uniform bool Mirror = false;
uniform float LensOffset = 0.0;
uniform float ViewportAspectRatio = 1.0;

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord0;

out vec2 vTexCoord;

void main() {
  vTexCoord = TexCoord0;
  // put the texture coordinates into the range [-1,1]
  vTexCoord *= 2.0;
  vTexCoord -= 1.0;
  // now correct for the lens offset
  float eyeLensOffset = LensOffset;
  eyeLensOffset *= (Mirror ? -1 : 1);
  vTexCoord.x -= eyeLensOffset;
  vTexCoord.y /= ViewportAspectRatio;

  gl_Position = vec4(Position, 1);
}
