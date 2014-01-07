#version 330

uniform sampler2D Scene;
uniform sampler2D OffsetMap;
uniform bool Mirror;

in vec2 vTexCoord;
out vec4 FragColor;

const vec2 ZERO = vec2(0);
const vec2 ONE = vec2(1);

void main() {
    vec2 texCoord = vTexCoord;
    if (Mirror) {
        texCoord.x = 1.0 - texCoord.x;
    }
    texCoord = texture(OffsetMap, texCoord).rg;
    vec2 clamped = clamp(texCoord, ZERO, ONE);
    if (!all(equal(texCoord, clamped))) {
        discard;
    }
    if (all(equal(texCoord, ZERO))) {
        discard;
    }
    if (Mirror) {
        texCoord.x = 1.0 - texCoord.x;
    }
    FragColor =  texture(Scene, texCoord);
}

