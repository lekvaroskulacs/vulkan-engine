#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "command_buffer.h"

namespace engine
{

class Uniform
{
public:
    virtual ~Uniform()
    {
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_device->destroyBuffer(m_uniformBuffers[i], m_uniformBuffersAllocations[i]);
        }
    }

    vk::DeviceSize getBufferSize()
    {
        return m_bufferSize;
    }

    void updateBuffer(void* data, int currentImage)
    {
        m_device->copyMemoryToAllocation(data, m_uniformBuffersAllocations[currentImage], m_bufferSize);
    }

    std::vector<vk::Buffer> m_uniformBuffers;
    std::vector<VmaAllocation> m_uniformBuffersAllocations;

protected:
    explicit Uniform(std::shared_ptr<Device> device, vk::DeviceSize bufferSize)
        : m_device{device}
    {
        m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_device->createBuffer(bufferSize,
                                   vk::BufferUsageFlagBits::eUniformBuffer,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                   m_uniformBuffers[i],
                                   m_uniformBuffersAllocations[i]);
        }
    };

    std::shared_ptr<Device> m_device;
    vk::DeviceSize m_bufferSize;
};

class UniformGameObject : public Uniform
{
public:
    struct UniformBufferObject
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    explicit UniformGameObject(std::shared_ptr<Device> device)
        : Uniform(device, sizeof(UniformBufferObject))
    {
        m_bufferSize = sizeof(UniformBufferObject);
    }
};

class UniformCamera : public Uniform
{
public:
    struct UniformBufferObject
    {
        alignas(16) glm::mat4 rayDir;
        alignas(16) glm::vec4 position;
    };

    explicit UniformCamera(std::shared_ptr<Device> device)
        : Uniform(device, sizeof(UniformBufferObject))
    {
        m_bufferSize = sizeof(UniformBufferObject);
    }
};

class UniformLight : public Uniform
{
public:
    struct UniformBufferObject
    {
        alignas(16) glm::vec4 position;
        alignas(16) glm::vec4 powerDensity;
        alignas(16) glm::mat4 shadowViewProj;
    };

    explicit UniformLight(std::shared_ptr<Device> device)
        : Uniform(device, sizeof(UniformBufferObject))
    {
        m_bufferSize = sizeof(UniformBufferObject);
    }
};

} // namespace engine