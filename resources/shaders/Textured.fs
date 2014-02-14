#version 330

uniform sampler2D sampler;

in vec2 vTexCoord;
out vec4 FragColor;

void main() {
  FragColor = texture(sampler, vTexCoord);
}
