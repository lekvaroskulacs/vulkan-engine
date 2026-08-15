#pragma once
#include <engine/uniforms/uniforms.h>

namespace engine {

struct Light
{
    alignas(16) glm::vec4 position;
    alignas(16) glm::vec4 colorIntensity;
};

// TODO: probably there should be an extra step in the hierarchy for StorageBuffer 
// when needed and light should inherit that
class LightBuffer : public ConcreteBuffer
{
public:
    static constexpr uint32_t MAX_LIGHTS = 64;

    struct LightBufferObject
    {
        alignas(16) uint32_t count;
        alignas(16) Light lights[MAX_LIGHTS];
    };

    explicit LightBuffer(std::shared_ptr<Device> device)
        : ConcreteBuffer(device, sizeof(LightBufferObject), vk::BufferUsageFlagBits::eStorageBuffer)
    {
        m_bufferSize = sizeof(LightBufferObject);
    }
};

}


