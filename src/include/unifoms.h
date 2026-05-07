#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "command_buffer.h"

namespace engine
{

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 rayDir;
};

class Uniform
{
public:
    explicit Uniform(std::shared_ptr<Device> device)
        : m_device{device} { };
    ~Uniform()
    {
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_device->destroyBuffer(m_uniformBuffers[i], m_uniformBuffersAllocations[i]);
        }
    }

    template <typename BufferObject>
    void setBufferObject()
    {
        vk::DeviceSize bufferSize = sizeof(BufferObject);

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
    }

    std::vector<vk::Buffer> m_uniformBuffers;
    std::vector<VmaAllocation> m_uniformBuffersAllocations;

private:
    std::shared_ptr<Device> m_device;
};

} // namespace engine