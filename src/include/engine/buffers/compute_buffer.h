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

class ClusterBoundsBuffer : ComputeBuffer
{
public:

    constexpr static uint32_t gridX = 16;
    constexpr static uint32_t gridY = 9;
    constexpr static uint32_t numSlices = 24; 

    class SSBO
    {
        utils::AABB[gridX * gridY * numSlices] m_aabb;
    };

    explicit ClusterBoundsBuffer(std::shared_ptr<Device> device)
        : ComputeBuffer{ device, sizeof(SSBO) }
    { 
        m_bufferSize = sizeof(SSBO);
    }
}

class LightGridBuffer
{
public:

    class SSBO
    {
        
    };
}


}

