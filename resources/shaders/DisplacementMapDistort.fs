uniform sampler2D Scene;
uniform sampler2D OffsetMap;
uniform bool Mirror;

// From the vertex shader
in vec2 vTexCoord;

const vec2 ZERO = vec2(0);
const vec2 ONE = vec2(1);
const vec2 Scale = vec2(0.5, 0.8 * 0.5);

vec4 finalColor(vec2 texCoord, float dx, float dy) {
    if (Mirror) {
        texCoord.x = 1.0 - texCoord.x;
    }
    texCoord = texture2D(OffsetMap, vec2(texCoord.x + dx, texCoord.y + dy)).xy;
    vec2 clamped = clamp(texCoord, ZERO, ONE);
    if (!all(equal(texCoord, clamped))) {
        discard;
    }
    if (Mirror) {
        texCoord.x = 1.0 - texCoord.x;
    }
    return texture2D(Scene, texCoord);
}

void main()
{
    vec2 texCoord = vTexCoord;
    float dx = dFdx(vTexCoord.x) / 4.5;
    float dy = dFdy(vTexCoord.y) / 4.5;
    vec4 color =
            finalColor(texCoord, dx, dy) +
            finalColor(texCoord, -dx, dy) +
            finalColor(texCoord, -dx, -dy) +
            finalColor(texCoord, dx, -dy);
    color /= 4.0;
    gl_FragColor = color;  //texture2D(Scene, texCoord);
    gl_FragColor = finalColor(texCoord, 0, 0);
//    gl_FragColor = vec4(0, 0, 1, 0);
}

