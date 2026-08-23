#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include <engine/command_buffer/command_buffer.h>

namespace engine
{

class UpdatableBuffer
{
public:
    virtual ~UpdatableBuffer() = default;
    virtual vk::DeviceSize getBufferSize() = 0;
    virtual void updateBuffer(void* data, int currentImage) = 0;
};

class ConcreteBuffer : public UpdatableBuffer
{
public:
    ~ConcreteBuffer() override
    {
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_device->destroyBuffer(m_buffers[i], m_bufferAllocations[i]);
        }
    }

    vk::DeviceSize getBufferSize() override
    {
        return m_bufferSize;
    }

    void updateBuffer(void* data, int currentImage) override
    {
        m_device->copyMemoryToAllocation(data, m_bufferAllocations[currentImage], m_bufferSize);
    }

    /// What kind of descriptor this buffer should be bound as (drives Pipeline's descriptor
    /// set layout/pool/write setup), inferred from the usage flags it was created with.
    vk::DescriptorType GetDescriptorType() const
    {
        return (m_usage & vk::BufferUsageFlagBits::eStorageBuffer) ? vk::DescriptorType::eStorageBuffer
                                                                    : vk::DescriptorType::eUniformBuffer;
    }

    std::vector<vk::Buffer> m_buffers;
    std::vector<VmaAllocation> m_bufferAllocations;

protected:
    explicit ConcreteBuffer(std::shared_ptr<Device> device,
                            vk::DeviceSize bufferSize,
                            vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eUniformBuffer)
        : m_device{device}
        , m_usage{usage}
    {
        m_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_bufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_device->createBuffer(bufferSize,
                                   usage,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                   m_buffers[i],
                                   m_bufferAllocations[i]);
        }
    };

    std::shared_ptr<Device> m_device;
    vk::DeviceSize m_bufferSize;
    vk::BufferUsageFlags m_usage;
};

class Uniform : public ConcreteBuffer
{
protected:
    explicit Uniform(std::shared_ptr<Device> device, vk::DeviceSize bufferSize)
        : ConcreteBuffer(device, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer)
    {
    }
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
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
        alignas(16) glm::vec4 nearFar; // x = near, y = far
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
        alignas(16) glm::mat4 shadowView;
        alignas(16) glm::mat4 shadowProj;
    };

    explicit UniformLight(std::shared_ptr<Device> device)
        : Uniform(device, sizeof(UniformBufferObject))
    {
        m_bufferSize = sizeof(UniformBufferObject);
    }
};

} // namespace engine
