// Shared cluster-grid layout for clustered forward rendering.
// cluster_build.comp writes ClusterBounds; light_cull.comp reads it and writes
// LightGrid + LightIndices; the forward fragment shader will read LightGrid +
// LightIndices to find which lights affect its cluster.
//
// Keep gridX/gridY/numSlices/avgLights in sync with the matching constants in
// src/include/engine/buffers/compute_buffer.h - buffer sizes are computed from
// those on the C++ side, so a mismatch here means overrunning the real buffer.

const uint gridX = 16;
const uint gridY = 9;
const uint numSlices = 24;
const uint numClusters = gridX * gridY * numSlices;

const uint avgLights = 3;

struct AABB
{
    vec4 min;
    vec4 max;
};

struct LightPerClusterProperties
{
    uint offset; // offset into LightIndices.indices
    uint count;
};

layout(set = 0, binding = 2) buffer ClusterBounds
{
    AABB aabb[numClusters];
} clusterBounds;

layout(set = 0, binding = 3) buffer LightGrid
{
    LightPerClusterProperties lightsPerCluster[numClusters];
} lightGrid;

layout(set = 0, binding = 4) buffer LightIndices
{
    uint currentIdx;
    uint indices[numClusters * avgLights];
} lightIndices;

// Every consumer (build, cull, forward shading) must agree on this mapping,
// so it lives here instead of being re-derived per shader.
uint clusterIndex(uvec3 coord)
{
    return coord.x + coord.y * gridX + coord.z * gridX * gridY;
}
