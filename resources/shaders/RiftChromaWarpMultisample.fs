#version 330

uniform sampler2DMS Scene;
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

    ivec2 texCoordRI = ivec2(textureSize(Scene) * texCoordR);
    ivec2 texCoordGI = ivec2(textureSize(Scene) * texCoordG);
    ivec2 texCoordBI = ivec2(textureSize(Scene) * texCoordB);
    FragColor.r = texelFetch(Scene, texCoordRI, 0).r;
    FragColor.b = texelFetch(Scene, texCoordGI, 0).g;
    FragColor.g = texelFetch(Scene, texCoordBI, 0).b;
    FragColor.a = 1.0;
}

