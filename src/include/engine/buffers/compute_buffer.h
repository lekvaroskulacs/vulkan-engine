#pragma once
#include <engine/uniforms/uniforms.h>
#include <engine/utils/types.h>

namespace engine {

/// Base for GPU-side storage buffers (SSBOs) written and/or read by compute shaders,
/// as opposed to the CPU-updated uniform buffers ConcreteBuffer's other subtype (Uniform) wraps.
class ComputeBuffer : public ConcreteBuffer
{
protected:
    explicit ComputeBuffer(std::shared_ptr<Device> device, vk::DeviceSize bufferSize)
        : ConcreteBuffer(device, bufferSize, vk::BufferUsageFlagBits::eStorageBuffer)
    {
    }
};

namespace {
constexpr uint32_t gridX = 16;
constexpr uint32_t gridY = 9;
constexpr uint32_t numSlices = 24;

constexpr uint32_t avgLights = 3;
}

class ClusterBoundsBuffer : public ComputeBuffer
{
public:
    struct SSBO
    {
        utils::AABB m_aabb[gridX * gridY * numSlices];
    };

    explicit ClusterBoundsBuffer(std::shared_ptr<Device> device)
        : ComputeBuffer{ device, sizeof(SSBO) }
    {
        m_bufferSize = sizeof(SSBO);
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
        LightPerClusterProperties m_lightsPerCluster[gridX * gridY * numSlices];
    };

    explicit LightGridBuffer(std::shared_ptr<Device> device)
        : ComputeBuffer{ device, sizeof(SSBO) }
    {
        m_bufferSize = sizeof(SSBO);
    }
};

class LightIndexBuffer : public ComputeBuffer
{
public:
    struct SSBO
    {
        uint32_t m_currentIndex = 0;
        uint32_t m_indices[gridX * gridY * numSlices * avgLights];
    };

    explicit LightIndexBuffer(std::shared_ptr<Device> device)
        : ComputeBuffer{ device, sizeof(SSBO) }
    {
        m_bufferSize = sizeof(SSBO);
    }
};

}
