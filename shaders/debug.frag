#version 450

layout(location = 0) out vec4 outColor;

layout(binding = 4) uniform sampler2D shadowMap;

float LinearizeDepth(float depth)
{
  float n = 0.1;
  float f = 1000.0;
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));	
}

void main() {
    float depth = texture(shadowMap, gl_FragCoord.xy).r;
    outColor = vec4(vec3(1.0 - LinearizeDepth(depth)), 1.0);
    //outColor = vec4(vec3(depth), 1.0);
}