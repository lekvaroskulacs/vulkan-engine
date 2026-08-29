#version 450

layout(location = 0) in vec4 worldPos;
layout(location = 1) in vec4 worldNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

#include "common/scene_light.glsl"

#define CLUSTER_BUFFER_QUALIFIER readonly
#include "common/cluster.glsl"

bool contains(AABB aabb, vec3 p)
{
    return all(greaterThanEqual(p, aabb.min.xyz)) && all(lessThanEqual(p, aabb.max.xyz));
}

void main()
{
    vec3 viewPos = (camera.view * worldPos).xyz;

    outColor = vec4(1.0);
    for (uint i = 0; i < numClusters; i++)
    {
        AABB aabb = clusterBounds.aabb[i];

        if (contains(aabb, viewPos))
        {
            vec3 color = vec3(float(i) / float(numClusters));
            outColor = vec4(color, 1.0);
        }
    }
}