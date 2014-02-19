#version 330

uniform sampler2D Scene;
uniform sampler2D OffsetMap;

in vec2 vTexCoord;
out vec4 FragColor;

const vec2 ZERO = vec2(0);
const vec2 ONE = vec2(1);

void main() {
    vec2 texCoord = vTexCoord;
    vec2 texCoordB = texture(OffsetMap, vTexCoord).ba;

    vec2 clamped = clamp(texCoordB, ZERO, ONE);
    if (!all(equal(texCoordB, clamped))) {
        discard;
    }

    vec2 texCoordR = texture(OffsetMap, vTexCoord).rg;
    vec2 texCoordG = (texCoordB + texCoordR) / 2.0;
    FragColor.r = texture(Scene, texCoordB).r;
    FragColor.b = texture(Scene, texCoordR).b;
    FragColor.g = texture(Scene, texCoordG).g;
    FragColor.a = 1.0;
}

