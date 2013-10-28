#version 330

uniform vec4 Ambient;
uniform vec4 LightPosition[8];
uniform vec4 LightColor[8];
uniform int LightCount;
uniform vec4 Color;

in vec3 vViewNormal;
in vec3 vViewPosition;

vec4 DoLight()
{
   vec3 norm = normalize(vViewNormal);
   vec3 light = Ambient.rgb;
   for (int i = 0; i < int(LightCount); i++)
   {
       vec3 ltp = (LightPosition[i].xyz - vViewPosition);
       float  ldist = length(ltp);
       ltp = normalize(ltp);
       light += clamp(LightColor[i].rgb * Color.rgb * (dot(norm, ltp) / ldist), 0.0,1.0);
   }
   return vec4(light, Color.a);
}

void main()
{
    gl_FragColor = (DoLight() * Color);
}
