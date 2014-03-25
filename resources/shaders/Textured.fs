#version 330

uniform sampler2D sampler;

in vec2 vTexCoord;
out vec4 vFragColor;

void main() {
  vFragColor = texture(sampler, vTexCoord);
}
