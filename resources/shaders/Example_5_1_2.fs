#version 330

uniform sampler2D sampler;
uniform bool Mirror;
uniform float ViewportAspectRatio;

in vec2 vTexCoord;
in vec2 vPosition;
out vec4 FragColor;

void main() {
  vec2 warpedTexCoord = vTexCoord;
  float len = length(warpedTexCoord) * 3.14159 / 2.0;
  len = cos(len);
  warpedTexCoord /= len;
  warpedTexCoord += 1.0;
  warpedTexCoord /= 2.0;

  if (!all(equal(clamp(warpedTexCoord, 0.0, 1.0), warpedTexCoord))) {
    FragColor = vec4(Mirror ? 0.4 : 0.0, 0.4, Mirror ? 0.0 : 0.4, 1.0);
  } else {
    FragColor = texture(sampler, warpedTexCoord);
  }
}

