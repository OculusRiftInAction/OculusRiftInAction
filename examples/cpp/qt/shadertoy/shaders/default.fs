#pragma vr
void main(void)
{
  vec2 uv = gl_FragCoord.xy / iResolution.xy;
  vec4 color = vec4(iDir, 1.0);
//  vec4 color = texture(iChannel0, iDir);
  gl_FragColor = color; 
}
