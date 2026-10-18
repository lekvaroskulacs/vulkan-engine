#pragma once
#include <engine/uniforms/uniforms.h>
#include <engine/utils/types.h>

namespace engine {

/// Base for GPU-side storage buffers (SSBOs) written and/or read by compute shaders,
/// as opposed to the CPU-updated uniform buffers ConcreteBuffer's other subtype (Uniform) wraps.
class ComputeBuffer : public ConcreteBuffer
{
protected:
    explicit ComputeBuffer(std::shared_ptr<Device> device, vk::DeviceSize bufferSize, vk::BufferUsageFlags extraUsage = {})
        : ConcreteBuffer(device, bufferSize, vk::BufferUsageFlagBits::eStorageBuffer | extraUsage)
    {
    }
};

namespace cluster {
constexpr uint32_t gridX = 16;
constexpr uint32_t gridY = 9;
constexpr uint32_t numSlices = 24;

constexpr uint32_t avgLights = 15;
}

class ClusterBoundsBuffer : public ComputeBuffer
{
public:
    struct SSBO
    {
        utils::AABB m_aabb[cluster::gridX * cluster::gridY * cluster::numSlices];
    };

    explicit ClusterBoundsBuffer(std::shared_ptr<Device> device)
        : ComputeBuffer{ device, sizeof(SSBO) }
    {
    }
};

struct LightPerClusterProperties
{
    alignas(4) uint32_t m_offset; // offset in the flat index buffer
    alignas(4) uint32_t m_count;
};

class LightGridBuffer : public ComputeBuffer
{
public:
    struct SSBO
    {
        LightPerClusterProperties m_lightsPerCluster[cluster::gridX * cluster::gridY * cluster::numSlices];
    };

    explicit LightGridBuffer(std::shared_ptr<Device> device)
        : ComputeBuffer{ device, sizeof(SSBO) }
    {
    }
};

class LightIndexBuffer : public ComputeBuffer
{
public:
    struct SSBO
    {
        uint32_t m_currentIndex = 0;
        uint32_t m_indices[cluster::gridX * cluster::gridY * cluster::numSlices * cluster::avgLights];
    };

    explicit LightIndexBuffer(std::shared_ptr<Device> device)
        : ComputeBuffer{ device, sizeof(SSBO), vk::BufferUsageFlagBits::eTransferDst }
    {
    }
};

namespace grass {
// Keep in sync with lodDistanceThreshold in shaders/compute/grass_near.comp and
// shaders/compute/grass_far.comp.
constexpr float lodDistanceThreshold = 10.0f;

namespace fine {
// Keep in sync with numTilesX/numTilesZ/tileSize/instancesPerTileAxis in
// shaders/compute/grass_near.comp.
constexpr uint32_t numTilesX = 20;
constexpr uint32_t numTilesZ = 20;
constexpr float tileSize = 10.0f;
constexpr uint32_t instancesPerTileAxis = 30; // local_size_x/y in grass_near.comp
constexpr uint32_t instanceCount = numTilesX * numTilesZ * instancesPerTileAxis * instancesPerTileAxis;
}

namespace coarse {
// Tiles are twice fine::tileSize with the same lane count per tile, so numTilesX * tileSize
// matches fine:: exactly (same world footprint) at a quarter of the blade density - like
// one mip level down. Keep in sync with shaders/compute/grass_far.comp.
constexpr uint32_t numTilesX = fine::numTilesX / 2;
constexpr uint32_t numTilesZ = fine::numTilesZ / 2;
constexpr float tileSize = fine::tileSize * 2.0f;
constexpr uint32_t instancesPerTileAxis = 30; // local_size_x/y in grass_far.comp
constexpr uint32_t instanceCount = numTilesX * numTilesZ * instancesPerTileAxis * instancesPerTileAxis;
}

constexpr uint32_t instanceCount = fine::instanceCount + coarse::instanceCount;
}

// Layout matches VkDrawIndirectCommand exactly, so this buffer can be written by
// grass.comp and then fed straight into vkCmdDrawIndirect as the indirect-args buffer.
class GrassDataBuffer : public ComputeBuffer
{
public:
    struct SSBO
    {
        alignas(4) uint32_t vertexCount;
        alignas(4) uint32_t instanceCount;
        alignas(4) uint32_t firstVertex;
        alignas(4) uint32_t firstInstance;
    };

    explicit GrassDataBuffer(std::shared_ptr<Device> device)
        : ComputeBuffer{ device, sizeof(SSBO), vk::BufferUsageFlagBits::eIndirectBuffer }
    {
    }
};


struct GrassInstanceData
{
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec2 facing;
    float height;
    float width;
    float tilt;
    float bend;
    float lod;
};

class GrassInstanceDataBuffer : public ComputeBuffer
{
public:
    struct SSBO
    {
        GrassInstanceData m_instances[grass::instanceCount];
    };

    explicit GrassInstanceDataBuffer(std::shared_ptr<Device> device)
        : ComputeBuffer{ device, sizeof(SSBO) }
    {
    }
};

}
