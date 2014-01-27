#version 330

uniform sampler2D sampler;

in vec2 vTexCoord;
out vec4 FragColor;

void main() {
  if (!all(equal(clamp(vTexCoord, 0.0, 1.0), vTexCoord))) {
    discard;
  }
  FragColor = texture(sampler, vTexCoord);
  //FragColor = vec4(vTexCoord, 0, 1);
}
