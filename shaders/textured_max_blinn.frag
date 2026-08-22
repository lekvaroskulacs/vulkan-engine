#version 450
#extension GL_GOOGLE_include_directive : require

#include "common/shading.glsl"

layout(location = 0) in vec4 worldPos;
layout(location = 1) in vec4 worldNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(set = 1, binding = 2) uniform Light {
     vec4 position;
     vec4 powerDensity;
} light;


void main() {
     vec3 normal = normalize(worldNormal.xyz);
     vec3 x = worldPos.xyz / worldPos.w;
     vec3 lightDir = normalize(light.position.xyz - x);
     vec3 lightDiff = light.position.xyz - x;
     float lightDiff2 = dot(lightDiff, lightDiff);
     vec3 viewDir = normalize(camera.position.xyz - x);

     vec3 texColor = texture(texSampler, fragTexCoord).xyz;
     vec3 radiance = iterateLights(x, normal);
     radiance += shade(light.powerDensity.xyz / lightDiff2, normal, lightDir, viewDir);
     outColor = vec4(texColor * radiance, 1.0);

}