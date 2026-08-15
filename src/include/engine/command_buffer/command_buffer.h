#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <engine/device/device.h>

namespace engine
{

class CommandBuffer
{
public:
    vk::CommandBuffer* GetBufferPtr(size_t idx);
    std::vector<vk::CommandBuffer> GetBuffers();

    explicit CommandBuffer(std::shared_ptr<Device> device);
    ~CommandBuffer();

    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

private:
    void createCommandPool();
    void createCommandBuffers();

    std::shared_ptr<Device> m_device;

    vk::CommandPool m_commandPool;
    std::vector<vk::CommandBuffer> m_commandBuffers;
};

} // namespace engine