#version 330

uniform vec2 LensCenter;
uniform vec2 Aspect;
uniform vec2 DistortionScale;
uniform vec4 K;

in vec2 vRiftTexCoord;

float lengthSquared(vec2 point) {
    point = point * point;
    return point.x + point.y;
}

float distortionFactor(vec2 point) {
    float rSq = lengthSquared(point);
    float factor =  (K[0] + K[1] * rSq + K[2] * rSq * rSq + K[3] * rSq * rSq * rSq);
    return factor;
}

// Adjust from lens origin coordinates to viewport origin
// coordinates
vec2 lensToScreen(vec2 point) {
    return point + LensCenter;
}

vec2 screenToTexture(vec2 point) {
    // Next, we adjust the axes to reflect the aspect ratio.  In
    // the case of the Rift, the per eye aspect is 4:5 or 0.8.  This means that
    // a pixel based Y coordinate value will be icreasing faster than the
    // texture coordinate (which goes from 0-1 regardless of the size of the
    // target geometry).  To counteract this, we multiply by the aspect ratio
    // (represented here as a vec2(1, 0.8) so that the Y coordinates approach
    // their maximum values more slowly.
    point *= Aspect;

    // Overall the distortion has a shrinking effect.  This can be counteracted
    // by  applying a scale determined by matching the effect of the distortion
    // against a fit point
    point *= DistortionScale;


    // Next we move from screen centered coordinate in the range -1,1
    // to texture based coordinates in the range 0,1.
    return (point + 1.0) / 2.0;
}

void main()
{
    // we recieve the texture coordinate in 'lens space'.  i.e. the 2d
    // coordinates represent the position of the texture coordinate using the
    // lens center as an offset
    vec2 distorted = vRiftTexCoord * distortionFactor(vRiftTexCoord);

    // In order to use the (now distorted) texture coordinates with the OpenGL
    // sample object, we need to put them back into texture space.
    vec2 screenCentered = lensToScreen(distorted);
    vec2 texCoord = screenToTexture(screenCentered);
    gl_FragColor = vec4(texCoord.xy, 0.0, 0.0);
}

