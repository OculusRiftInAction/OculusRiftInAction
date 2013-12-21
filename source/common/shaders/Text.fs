#version 330

uniform sampler2D Font;
uniform vec4 Color;

in vec2 vTexCoord;
out vec4 FragColor;

const float smoothness = 1.0 / 1.0;
const float gamma = 2.6;
const float smoothing = 1.0/16.0;

void main()
{
   // retrieve signed distance
   float sdf = texture( Font, vTexCoord ).r;

   // perform adaptive anti-aliasing of the edges
   float w = clamp( smoothness * (abs(dFdx(vTexCoord.x)) + abs(dFdy(vTexCoord.y))), 0.0, 0.5);
   float a = smoothstep(0.5-w, 0.5+w, sdf);

   // gamma correction for linear attenuation
   a = pow(a, 1.0/gamma);

   if (a < 0.01) {
       discard;
   }

   // final color
   FragColor = vec4(1, 0, 0, 1);
   FragColor = vec4(Color.rgb, a);
}

void main1() {
    float distance = texture( Font, vTexCoord ).r;
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
//    gl_FragColor = vec4(Color.rgb, alpha);
//     gl_FragColor = vec4(1, 0, 0,1 );
}
