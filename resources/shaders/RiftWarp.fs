#version 330

uniform sampler2D Scene;
uniform sampler2D OffsetMap;

in vec2 vTexCoord;
out vec4 FragColor;

const vec2 ZERO = vec2(0);
const vec2 ONE = vec2(1);

void main() {
    vec2 texCoord = vTexCoord;
    texCoord = texture(OffsetMap, texCoord).rg;
    vec2 clamped = clamp(texCoord, ZERO, ONE);
    if (!all(equal(texCoord, clamped))) {
        discard;
    }
    if (all(equal(texCoord, ZERO))) {
        discard;
    }
    FragColor =  texture(Scene, texCoord);
}

