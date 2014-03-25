#version 330

uniform sampler2D Scene;

uniform float Aspect = 1.0;
uniform float LensOffset = 0.0;
uniform vec4 K = vec4(1, 0, 0, 0);

const vec2 ZERO = vec2(0);
const vec2 ONE = vec2(1);

in vec2 vTexCoord;

out vec4 vFragColor;

float undistortionScale(vec2 point) {
  float rSq = point.x * point.x + point.y * point.y;
  return K[0] + rSq * (K[1] + rSq * (K[2] + rSq * K[3]));
}

vec2 textureToScreen(vec2 v) {
  return (v * 2.0) - 1.0;
}

vec2 screenToTexture(vec2 v) {
  return (v + 1.0) / 2.0;
}

vec2 screenToRift(vec2 v) {
  v.x += LensOffset;
  v.y /= Aspect;
  return v;
}

vec2 riftToScreen(vec2 v) {
  v.y *= Aspect;
  v.x -= LensOffset;
  return v;
}

void main() {
  vec2 screenDistorted = textureToScreen(vTexCoord);
  vec2 riftDistorted = screenToRift(screenDistorted);
  float scale = undistortionScale(riftDistorted);
  vec2 riftUndistorted = scale * riftDistorted;
  vec2 screenUndistorted = riftToScreen(riftUndistorted);
  vec2 textureUndistorted = screenToTexture(screenUndistorted);

  if (!all(equal(textureUndistorted, 
      clamp(textureUndistorted, ZERO, ONE)))) {
    discard;
  }
  
  vFragColor = texture(Scene, textureUndistorted);
}

