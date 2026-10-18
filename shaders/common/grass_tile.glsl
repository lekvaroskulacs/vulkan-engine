// Shared by grass_near.comp/grass_far.comp - not included by grass.vert, since this writes
// to grassInstanceBuffer (from common/grass.glsl), which grass.vert includes as readonly.
// Include common/noise.glsl and common/grass.glsl before this file.



// Keep in sync with numVerticesLOD0 in shaders/grass.vert. A single indirect draw call has
// exactly one vertexCount shared by every instance, so this must always cover the larger
// LOD0 geometry - grass.vert clamps its vertex index back into range for LOD1 instances
// instead of this being varied per instance (which isn't possible for one draw call).
const uint vertexCountLOD0 = 39;

// Base (near/fine) tile grid - keep in sync with grass::fine::* in
// src/include/engine/buffers/compute_buffer.h.
const uint numTilesXNear = 20;
const uint numTilesZNear = 20;
const float tileSizeNear = 10.0;
const uint instancesPerTileAxis = 30;

#ifdef GRASS_FAR
// Far/coarse grid: half as many tiles per axis, twice the tile size - same world footprint
// as the near grid (numTilesX * tileSize matches) at a quarter of the blade density, like
// one mip level down. Its instances are appended right after all of the near grid's in the
// flat GrassInstanceData array. Keep in sync with grass::coarse::* in compute_buffer.h.
const uint numTilesX = numTilesXNear / 2;
const uint numTilesZ = numTilesZNear / 2;
const float tileSize = tileSizeNear * 2.0;
const uint instanceOffset = numTilesXNear * numTilesZNear * instancesPerTileAxis * instancesPerTileAxis;
const float lod = 1.0;
#else
const uint numTilesX = numTilesXNear;
const uint numTilesZ = numTilesZNear;
const float tileSize = tileSizeNear;
const uint instanceOffset = 0;
const float lod = 0.0;
#endif

const uint totalInstanceCount = (numTilesXNear * numTilesZNear + (numTilesXNear / 2) * (numTilesZNear / 2)) * instancesPerTileAxis * instancesPerTileAxis;

// Keep in sync with grass::lodDistanceThreshold in compute_buffer.h. See placeGrassInstance
// below for how this boundary is enforced.
const float lodDistanceThreshold = 10.0;

vec2 randomUnit(uint seed)
{
    uint hx = noiseHash(seed * 2654435761u + 1u);
    uint hy = noiseHash(seed * 2654435761u + 0x9e3779b9u);
    return vec2(float(hx), float(hy)) / 4294967296.0;
}

float randomFloat(uint seed, uint saltOffset, float minVal, float maxVal)
{
    float t = float(noiseHash(seed * 2654435761u + saltOffset)) / 4294967296.0;
    return mix(minVal, maxVal, t);
}

// Places one grass instance on a tile grid. tileCoord/localCoord come from
// gl_WorkGroupID.xy/gl_LocalInvocationID.xy (one workgroup per tile); flatIndex is this
// instance's slot in grassInstanceData, already offset for whichever pass is calling this.
// lod is 0 for the fine/near grid or 1 for the coarse/far grid: it decides which side of
// lodDistanceThreshold this grid is responsible for (near keeps points within it, far keeps
// points beyond it) - the other side is suppressed (width = 0, collapsing every one of the
// blade's quads to a zero-area sliver) so exactly one grid ever draws a given point.
void placeGrassInstance(uvec2 tileCoord,
                        uvec2 localCoord,
                        uint flatIndex,
                        float tileSize,
                        uint numTilesX,
                        uint numTilesZ,
                        uint instancesPerTileAxis,
                        float lod,
                        float lodDistanceThreshold,
                        vec3 cameraPosition)
{
    float cellSize = tileSize / float(instancesPerTileAxis);
    vec2 worldOrigin = vec2(-float(numTilesX) * tileSize * 0.5, -float(numTilesZ) * tileSize * 0.5);
    vec2 cellCenter = (vec2(localCoord) + 0.5) * cellSize;
    vec2 jitter = (randomUnit(flatIndex) - 0.5) * cellSize;
    vec2 positionXZ = worldOrigin + vec2(tileCoord) * tileSize + cellCenter + jitter;

    vec3 position = vec3(positionXZ.x, sampleHeight(positionXZ), positionXZ.y);

    float facingAngle = float(noiseHash(flatIndex * 2654435761u + 0x85ebca6bu)) / 4294967296.0 * 6.2831853;
    vec2 facing = vec2(cos(facingAngle), sin(facingAngle));
    float height = randomFloat(flatIndex, 0x27d4eb2fu, 0.7, 1.3);
    float width = randomFloat(flatIndex, 0x165667b1u, 0.1, 0.12);
    float tilt = randomFloat(flatIndex, 0xd3a2646cu, radians(50.0), radians(70.0));
    float bend = 0.5;

    bool pointIsFar = length(cameraPosition - position) > lodDistanceThreshold;
    bool thisGridIsFar = lod > 0.5;
    if (pointIsFar != thisGridIsFar)
    {
        width = 0.0;
    }

    grassInstanceBuffer.grassInstanceData[flatIndex].position = position;
    grassInstanceBuffer.grassInstanceData[flatIndex].facing = facing;
    grassInstanceBuffer.grassInstanceData[flatIndex].height = height;
    grassInstanceBuffer.grassInstanceData[flatIndex].width = width;
    grassInstanceBuffer.grassInstanceData[flatIndex].tilt = tilt;
    grassInstanceBuffer.grassInstanceData[flatIndex].bend = bend;
    grassInstanceBuffer.grassInstanceData[flatIndex].lod = lod;
}
