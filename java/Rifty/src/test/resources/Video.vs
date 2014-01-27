uniform bool uMirror;
uniform mat4 uProjection;

attribute vec4 aPosition;
attribute vec2 aTexCoord;

varying vec4 vPosition;
varying vec2 vTexCoord;

const float LensOffset = 0.15;
const float targetAspect = 640.0 / 800.0;

void main() {
    vPosition = aPosition;
    vTexCoord = aTexCoord;
    gl_Position = uProjection * vPosition;
}

