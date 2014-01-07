#version 330

uniform sampler2D sampler;

in vec2 vTexCoord;
out vec4 FragColor;

// based on http://www.gamerendering.com/2008/10/11/gaussian-blur-filter-shader/
const float blurSize = 1.0/128.0;
void main() {
  vec4 sum = vec4(0.0);

  // take nine samples, with the distance blurSize between them
  sum += texture(sampler, vec2(vTexCoord.x - 4.0*blurSize, vTexCoord.y)) * 0.05;
  sum += texture(sampler, vec2(vTexCoord.x - 3.0*blurSize, vTexCoord.y)) * 0.09;
  sum += texture(sampler, vec2(vTexCoord.x - 2.0*blurSize, vTexCoord.y)) * 0.12;
  sum += texture(sampler, vec2(vTexCoord.x - blurSize, vTexCoord.y)) * 0.15;
  sum += texture(sampler, vec2(vTexCoord.x, vTexCoord.y)) * 0.16;
  sum += texture(sampler, vec2(vTexCoord.x + blurSize, vTexCoord.y)) * 0.15;
  sum += texture(sampler, vec2(vTexCoord.x + 2.0*blurSize, vTexCoord.y)) * 0.12;
  sum += texture(sampler, vec2(vTexCoord.x + 3.0*blurSize, vTexCoord.y)) * 0.09;
  sum += texture(sampler, vec2(vTexCoord.x + 4.0*blurSize, vTexCoord.y)) * 0.05;

  FragColor = sum;
}

