uniform vec2 LensCenter;
uniform vec2 Aspect;
uniform vec2 DistortionScale;
uniform vec4 K;

const vec2 ZERO = vec2(0);
const vec2 ONE = vec2(1);
const vec2 Scale = vec2(0.5, 0.8 * 0.5);

uniform sampler2D sampler;

// From the vertex shader
varying vec2 vTexCoord;


float lengthSquared(vec2 point) {
    point = point * point;
    return point.x + point.y;
}

float distortionFactor(vec2 point) {
    float rSq = lengthSquared(point);
    float factor =  (K[0] + K[1] * rSq + K[2] * rSq * rSq + K[3] * rSq * rSq * rSq);
    return factor;
}

void main()
{
	// we recieve the texture coordinate in 'lens space'.  i.e. the 2d 
	// coordinates represent the position of the texture coordinate using the 
	// lens center as an offset
    vec2 distorted = vTexCoord * distortionFactor(vTexCoord);

	// In order to use the (now distorted) texture coordinates with the OpenGL 
	// sample object, we need to put them back into texture space.  

	// First, we want to adjust from lens origin coordinates to viewport origin 
	// coordinates
	vec2 screenCentered = distorted + LensCenter;

	// Next, we adjust the axes to reflect the aspect ratio.  In
	// the case of the Rift, the per eye aspect is 4:5 or 0.8.  This means that 
	// a pixel based Y coordinate value will be icreasing faster than the 
	// texture coordinate (which goes from 0-1 regardless of the size of the 
	// target geometry).  To counteract this, we multiply by the aspect ratio 
	// (represented here as a vec2(1, 0.8) so that the Y coordinates approach 
	// their maximum values more slowly.  
	screenCentered *= Aspect;

	// Overall the distortion has a shrinking effect.  This can be counteracted 
	// by  applying a scale determined by matching the effect of the distortion 
	// against a fit point
	screenCentered *= DistortionScale;
	
	// Next we move from screen centered coordinate in the range -1,1
	// to texture based coordinates in the range 0,1.  
	vec2 textureCoord = (screenCentered + 1.0) / 2.0;

	vec2 clamped = clamp(textureCoord, ZERO, ONE);
	if (!all(equal(textureCoord, clamped))) {
//		discard;
		gl_FragColor = vec4(0.5, 0.0, 0.0, 1.0);
	} else {
		gl_FragColor = texture2D(sampler, textureCoord);
	    //gl_FragColor = vec4(abs(textureCoord), 0.0, 1.0);
	}
}

