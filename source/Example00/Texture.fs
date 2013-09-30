uniform sampler2D sampler;
varying vec2 oTexCoord;

void main()
{
   gl_FragColor = texture2D(sampler, oTexCoord);
//   gl_FragColor = vec4(oTexCoord, 0, 1);
}
