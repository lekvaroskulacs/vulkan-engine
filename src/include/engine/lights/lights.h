#pragma once
#include <engine/buffers/compute_buffer.h>

namespace engine {

struct Light
{
    alignas(16) glm::vec4 position;
    alignas(16) glm::vec4 colorIntensity;
    alignas(4) float radius;
};

class LightBuffer : public ComputeBuffer
{
public:
    static constexpr uint32_t MAX_LIGHTS = 256;

    struct LightBufferObject
    {
        alignas(16) uint32_t count;
        alignas(16) Light lights[MAX_LIGHTS];
    };

    explicit LightBuffer(std::shared_ptr<Device> device)
        : ComputeBuffer(device, sizeof(LightBufferObject))
    {
        m_bufferSize = sizeof(LightBufferObject);
    }
};

}


