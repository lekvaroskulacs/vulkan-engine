#version 450

#include "common/shading.glsl"

layout(location = 0) in vec4 worldPos;
layout(location = 1) in vec4 worldNormal;
layout(location = 0) out vec4 outColor;

void main()
{
    vec3 normal = normalize(worldNormal.xyz);
    if (!gl_FrontFacing)
    {
        normal = -normal;
    }
    vec3 x = worldPos.xyz / worldPos.w;

    vec3 ambient = vec3(0.0, 0.9, 0.61);
    vec3 radiance = iterateLightsClustered(x, normal);
    outColor = vec4(ambient * radiance, 1.0);
}