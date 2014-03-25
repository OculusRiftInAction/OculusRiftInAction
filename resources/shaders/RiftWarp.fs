#version 330

uniform sampler2D Scene;
uniform sampler2D OffsetMap;

in vec2 vTexCoord;

out vec4 vFragColor;

const vec2 ZERO = vec2(0);
const vec2 ONE = vec2(1);

void main() {
  vec2 undistorted = 
      texture(OffsetMap, vTexCoord).rg;

  if (!all(equal(undistorted,
      clamp(undistorted, ZERO, ONE)))) {
    discard;
  }
  
  vFragColor =  texture(Scene, undistorted);
}
