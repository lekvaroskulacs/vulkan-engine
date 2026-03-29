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
};

class Uniforms
{
public:
    explicit Uniforms(std::shared_ptr<Device> device)
        : m_device{device}
    {
        createUniformBuffers();
    }

    ~Uniforms()
    {
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(m_device->GetDevice(), m_uniformBuffers[i], nullptr);
            vkFreeMemory(m_device->GetDevice(), m_uniformBuffersMemory[i], nullptr);
        }
    }

    void createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_device->createBuffer(bufferSize,
                                   vk::BufferUsageFlagBits::eUniformBuffer,
                                   vk::MemoryPropertyFlagBits::eHostVisible |
                                       vk::MemoryPropertyFlagBits::eHostCoherent,
                                   m_uniformBuffers[i],
                                   m_uniformBuffersMemory[i]);

            vkMapMemory(m_device->GetDevice(),
                        m_uniformBuffersMemory[i],
                        0,
                        bufferSize,
                        0,
                        &m_uniformBuffersMapped[i]);
        }
    }

    std::vector<vk::Buffer> m_uniformBuffers;
    std::vector<vk::DeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

private:
    std::shared_ptr<Device> m_device;
};

} // namespace engine