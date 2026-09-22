#ifndef GRASS_BUFFER_QUALIFIER
#define GRASS_BUFFER_QUALIFIER
#endif


struct GrassInstanceData
{
    vec3 position;
    vec2 facing;
};

// Layout matches VkDrawIndirectCommand so this buffer can be bound directly as the
// indirect-args buffer for vkCmdDrawIndirect.
layout (set = 0, binding = 5) GRASS_BUFFER_QUALIFIER buffer GrassData
{
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint firstInstance;
} grassData;

layout (set = 0, binding = 6) GRASS_BUFFER_QUALIFIER buffer GrassInstanceBuffer
{
    GrassInstanceData grassInstanceData[];
} grassInstanceBuffer;