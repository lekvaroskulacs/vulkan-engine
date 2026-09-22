#version 450

#include "common/noise.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec2 fragTexCoord;

layout(set = 1, binding = 0) uniform GameObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

const int noiseOctaves = 2;
const float noiseScale = 0.05; // controls the "wavelength" of terrain features, in world units
const float heightScale = 6.0;
const float normalSampleEpsilon = 0.1; // finite-difference step, in world-space units (same
                                        // units sampleHeight's output is in, unlike texcoord
                                        // space - that mismatch was the cause of the black spots)

float sampleHeight(vec2 worldXZ)
{
    return fbmNoise(worldXZ * noiseScale, noiseOctaves) * heightScale;
}

void main() {
    vec3 displacedPos = inPosition;
    displacedPos.y += sampleHeight(inPosition.xz);

    vec4 worldPos = ubo.model * vec4(displacedPos, 1.0);
    fragTexCoord = inTexCoord;
    outPos = worldPos;

    // The normal map is gone too - estimate the surface normal from the height field's local
    // slope via finite differences instead. This runs per-vertex rather than per-pixel, so a
    // few extra noise samples here are cheap compared to a fragment-shader equivalent would be.
    float heightDX = sampleHeight(inPosition.xz + vec2(normalSampleEpsilon, 0.0))
                    - sampleHeight(inPosition.xz - vec2(normalSampleEpsilon, 0.0));
    float heightDZ = sampleHeight(inPosition.xz + vec2(0.0, normalSampleEpsilon))
                    - sampleHeight(inPosition.xz - vec2(0.0, normalSampleEpsilon));
    vec3 normal = normalize(vec3(-heightDX, 2.0 * normalSampleEpsilon, -heightDZ));

    vec4 worldNormal = vec4(normal, 0.0) * inverse(ubo.model);
    outNormal = worldNormal;

    gl_Position = ubo.proj * ubo.view * worldPos;
}
