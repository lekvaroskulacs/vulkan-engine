#version 450
#extension GL_GOOGLE_include_directive : require

#include "common/shading.glsl"

layout(location = 0) in vec4 worldPos;
layout(location = 1) in vec4 worldNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

void main() {
     vec3 normal = normalize(worldNormal.xyz);
     vec3 x = worldPos.xyz / worldPos.w;

     vec3 texColor = vec3(0.64, 0.61, 0.61);
     vec3 radiance = iterateLightsClustered(x, normal);
     //radiance += shade(light.powerDensity.xyz / lightDiff2, normal, lightDir, viewDir);
     outColor = vec4(texColor * radiance, 1.0);
}