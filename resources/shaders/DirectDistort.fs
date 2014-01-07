#version 330

uniform sampler2D Scene;

uniform float Aspect = 1.0;
uniform float PostDistortionScale = 1.0;
uniform float LensOffset = 0.0;
uniform vec4 K = vec4(1, 0, 0, 0);

const vec2 ZERO = vec2(0);
const vec2 ONE = vec2(1);

in vec2 vRiftTexCoord;

out vec4 vFragColor;

float lengthSquared(vec2 point) {
  point = point * point;
  return point.x + point.y;
}

float distortionFactor(vec2 point) {
  float rSq = lengthSquared(point);
  return K[0] + rSq * (K[1] + rSq * (K[2] + rSq * K[3]));
}

// Adjust from lens origin coordinates to viewport origin coordinates
vec2 lensToScreen(vec2 point) {
  point.x += LensOffset;
  return point;
}

vec2 screenToTexture(vec2 point) {
  // Next, we adjust the axes to reflect the aspect ratio.  In the case of
  // the Rift, the per eye aspect is 4:5 or 0.8.  This means that a pixel
  // based Y coordinate value will be icreasing faster than the texture
  // coordinate (which goes from 0-1 regardless of the size of the target
  // geometry).  To counteract this, we multiply by the aspect ratio
  // (represented here as a vec2(1, 0.8) so that the Y coordinates approach
  // their maximum values more slowly.
  point.y *= Aspect;

  // Overall the distortion has a shrinking effect.  This is be counteracted
  // by  applying a scale determined by matching the effect of the distortion
  // against a fit point
  point *= PostDistortionScale;

  // Next we move from screen centered coordinate in the range -1,1
  // to texture based coordinates in the range 0,1.
  return (point + 1.0) / 2.0;
}

void main() {
  // we recieve the texture coordinate in 'lens space'.  i.e. the 2d
  // coordinates represent the position of the texture coordinate using the
  // lens center as an offset
  vec2 distorted = vRiftTexCoord * distortionFactor(vRiftTexCoord);

  // In order to use the (now distorted) texture coordinates with the OpenGL
  // sample object, we need to put them back into texture space.
  vec2 screenCentered = lensToScreen(distorted);

  vec2 texCoord = screenToTexture(screenCentered);

  vec2 clamped = clamp(texCoord, ZERO, ONE);
  if (!all(equal(texCoord, clamped))) {
    vFragColor = vec4(0.5, 0.5, 0.5, 1.0);
    return;
  }

  vFragColor = texture(Scene, texCoord);
}

