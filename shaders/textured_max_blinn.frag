#version 450
#extension GL_GOOGLE_include_directive : require

#include "common/shading.glsl"

layout(location = 0) in vec4 worldPos;
layout(location = 1) in vec4 worldNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(set = 1, binding = 2) uniform Material {
     vec2 offset;
     vec2 tiling;
     float roughness;
} material;

// Local to this shader rather than added to shading.glsl's shared brdf()/shade()/
// iterateLights(): those are also called from max_blinn.frag, grass.frag and
// omni_shadow.frag, none of which have a roughness value, and GLSL has no default
// parameters to add one without forcing every one of those call sites to start passing
// something. If more shaders end up needing this, promoting it into shading.glsl as a new
// addition (not a change to the existing functions) would make sense then.
vec3 brdfRoughness(vec3 normal, vec3 lightDir, vec3 viewDir, float roughness) {
    float cosa = dot(lightDir, normal);
    float cosb = dot(viewDir, normal);

    // roughness = 0 reproduces brdf()'s original fixed 10.0*128.0 exponent (mirror-sharp);
    // roughness = 1 relaxes it to a broad, dull highlight.
    float shininess = mix(4.0, 10.0 * 128.0, pow(clamp(1.0 - roughness, 0.0, 1.0), 2.0));

    return
        vec3(0.6, 0.6, 0.2) / pi +
        pow(clamp(dot(normalize(viewDir + lightDir), normal), 0.0, 1.0), shininess) * vec3(1.0, 1.0, 1.0) / max(cosa, cosb)
        ;
}

vec3 shadeRoughness(vec3 powerDensity, vec3 normal, vec3 lightDir, vec3 viewDir, float roughness) {
    float cosa = clamp(dot(lightDir, normal), 0.0, 1.0);
    return powerDensity * cosa * brdfRoughness(normal, lightDir, viewDir, roughness);
}

vec3 iterateLightsRoughness(vec3 worldPosition, vec3 normal, float roughness)
{
    vec3 viewDir = normalize(camera.position.xyz - worldPosition);
    vec3 radiance = vec3(0.0);
    for (int i = 0; i < sceneLights.count; i++)
    {
        vec3 lightDiff = sceneLights.lights[i].position.xyz - worldPosition * sceneLights.lights[i].position.w;
        vec3 lightDir = normalize(lightDiff);
        float lightDist2 = dot(lightDiff, lightDiff);

        float radius = sceneLights.lights[i].radius;
        float window = clamp(1.0 - pow(lightDist2 / (radius * radius), 2.0), 0.0, 1.0);
        float atten = (window * window) / lightDist2;
        if (sceneLights.lights[i].position.w <= 0.01)
        {
            atten = 1.0;
        }

        radiance += shadeRoughness(sceneLights.lights[i].colorIntensity.xyz * atten, normal, lightDir, viewDir, roughness);
    }
    return radiance;
}

void main() {
     vec3 normal = normalize(worldNormal.xyz);
     vec3 x = worldPos.xyz / worldPos.w;

     vec2 uv = fragTexCoord * material.tiling + material.offset;
     vec3 texColor = texture(texSampler, uv).xyz;
     vec3 radiance = iterateLightsRoughness(x, normal, material.roughness);
     outColor = vec4(texColor * radiance, 1.0);
}