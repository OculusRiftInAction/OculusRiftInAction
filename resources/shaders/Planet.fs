#version 330

uniform mat4 Projection = mat4(1);
uniform mat4 ModelView = mat4(1);
uniform mat3 NormalMatrix = mat3(1);

uniform sampler2D Color;
uniform sampler2D Night;
uniform sampler2D Bump;
uniform sampler2D Specular;
uniform sampler2D Clouds;

uniform vec4 SunColor = vec4(1, 1, 1, 1);
uniform vec3 SunVector = vec3(0, 1, 0);
uniform vec3 HalfVector = vec3(0, 1, 0);
uniform int Time = 0;

in vec3 vViewNormal;
in vec4 vViewPosition;
in vec2 vTexCoord;
out vec4 FragColor;

const int orbitMillis = 90 * 60 * 1000;
void main()
{
  vec2 texCoord = vTexCoord; {
    float xoffset = float(Time % orbitMillis) / orbitMillis;
    texCoord.x += xoffset;
  }

  float cloudAlpha = texture(Clouds, texCoord).r;

  float diffuse = dot(vViewNormal, SunVector);
  diffuse = pow(diffuse, 0.8);
  //diffuse = smoothstep(0, 1.05, diffuse) - 0.05;
  if (diffuse > 0) {
    float specular = 1 - texture(Specular, texCoord).r;
    vec3 surfaceColor;
    if (specular > 0.55) {
      specular *= dot(vViewNormal, HalfVector);
      specular = pow(specular, 10);
      // Reflected sun color
      vec3 oceanColor = vec3(0.04, 0.34, 0.83);
      vec3 specColor = vec3(0.98, 0.83, 0.28);
      oceanColor = mix(oceanColor, vec3(1), cloudAlpha);
      specular = mix(specular, 0, cloudAlpha);
//      vec3 renderColor = mix(oceanColor * diffuse, specColor, specular);
      FragColor = vec4(oceanColor * diffuse, 1);
    } else {
      vec3 color = texture(Color, texCoord).rgb;
      color = mix(color, vec3(1), cloudAlpha);
      color *= diffuse;
      FragColor = vec4(color, 1);
    }
  } else {
    FragColor = vec4(0, 0, 0, 1);
  }
  return;
  
  
  /*
  if (specularMask < 0.1) {
    float specular = dot(vViewNormal, HalfVector);
    specularMap = 1 - specularMap;
    specularMap *= specular;
    specularMap *= cloudAlpha;
  }
  vec3 specularMap = texture(Specular, texCoord).rgb;
  float cloudAlpha = pow(1 - clouds.r, 3);
  */

  vec3 day = texture(Color, texCoord).rgb;
  vec3 night = texture(Night, texCoord).rgb;
  
  
  vec3 color = vec3(0);
  if (diffuse > 0.0) {
//    color = (cloudAlpha * day);
//    color += clouds;
//    color = specularMap;
    color *= diffuse;
//    color += specularMap;
//    color = min(vec3(1), color);
//    color = min(vec3(1), (color + specularMap));
  } else {
//    color = cloudAlpha * night * abs(diffuse);
  }
  FragColor = vec4(color, 1);
  //FragColor = vec4(, 0, 0, 1);
}
