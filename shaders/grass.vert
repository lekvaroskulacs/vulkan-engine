#version 450

#define GRASS_BUFFER_QUALIFIER readonly
#include "common/scene_light.glsl"
#include "common/grass.glsl"

const float bladeHeight = 1.0;
const float baseHalfWidth = 0.03;
const int numSegments = 6;
const float segmentHeight = bladeHeight / float(numSegments);

// Row i's half-width shrinks toward (but doesn't reach) zero by the last row; the actual
// point is the separate apex vertex, reached via the closing triangle instead of a
// degenerate zero-width quad.
const vec3 bladeVertices[15] = vec3[15](
    // Row 0 (base)
    vec3(-baseHalfWidth * (1.0 - 0.0 / 7.0), 0.0 * segmentHeight, 0.0),
    vec3( baseHalfWidth * (1.0 - 0.0 / 7.0), 0.0 * segmentHeight, 0.0),
    // Row 1
    vec3(-baseHalfWidth * (1.0 - 1.0 / 7.0), 1.0 * segmentHeight, 0.0),
    vec3( baseHalfWidth * (1.0 - 1.0 / 7.0), 1.0 * segmentHeight, 0.0),
    // Row 2
    vec3(-baseHalfWidth * (1.0 - 2.0 / 7.0), 2.0 * segmentHeight, 0.0),
    vec3( baseHalfWidth * (1.0 - 2.0 / 7.0), 2.0 * segmentHeight, 0.0),
    // Row 3
    vec3(-baseHalfWidth * (1.0 - 3.0 / 7.0), 3.0 * segmentHeight, 0.0),
    vec3( baseHalfWidth * (1.0 - 3.0 / 7.0), 3.0 * segmentHeight, 0.0),
    // Row 4
    vec3(-baseHalfWidth * (1.0 - 4.0 / 7.0), 4.0 * segmentHeight, 0.0),
    vec3( baseHalfWidth * (1.0 - 4.0 / 7.0), 4.0 * segmentHeight, 0.0),
    // Row 5
    vec3(-baseHalfWidth * (1.0 - 5.0 / 7.0), 5.0 * segmentHeight, 0.0),
    vec3( baseHalfWidth * (1.0 - 5.0 / 7.0), 5.0 * segmentHeight, 0.0),
    // Row 6
    vec3(-baseHalfWidth * (1.0 - 6.0 / 7.0), 6.0 * segmentHeight, 0.0),
    vec3( baseHalfWidth * (1.0 - 6.0 / 7.0), 6.0 * segmentHeight, 0.0),
    // Apex
    vec3(0.0, bladeHeight, 0.0)
);

// Two triangles per quad (rows 0-1 through 5-6), plus one closing triangle for the apex.
// 6 quads * 2 + 1 = 13 triangles = 39 indices. Winding is CCW as seen from +Z; flip if
// backface culling shows the wrong side once this is wired into the actual pipeline.
const int bladeIndices[39] = int[39](
    0, 1, 2,    1, 3, 2,
    2, 3, 4,    3, 5, 4,
    4, 5, 6,    5, 7, 6,
    6, 7, 8,    7, 9, 8,
    8, 9, 10,   9, 11, 10,
    10, 11, 12, 11, 13, 12,
    12, 13, 14
);

const vec3 bladeNormal = normalize(cross(normalize(bladeVertices[1] - bladeVertices[0]),
                                         normalize(bladeVertices[2] - bladeVertices[0])));


const float maxRoundAngle = radians(75.0);

vec3 roundedBladeNormal(float localX)
{
    float t = clamp(localX / baseHalfWidth, -1.0, 1.0);
    float theta = t * maxRoundAngle;
    return normalize(vec3(sin(theta), 0.0, cos(theta) * bladeNormal.z));
}

layout(location = 0) out vec4 outWorldPos;
layout(location = 1) out vec4 outWorldNormal;


void main()
{
    vec3 localPos = bladeVertices[bladeIndices[gl_VertexIndex]];
    vec3 localNormal = roundedBladeNormal(localPos.x);
    vec4 worldPos = vec4(localPos + grassInstanceBuffer.grassInstanceData[gl_InstanceIndex].position, 1.0);
    gl_Position = camera.proj * camera.view * worldPos;

    outWorldPos = worldPos;
    outWorldNormal = vec4(localNormal, 0.0);
}