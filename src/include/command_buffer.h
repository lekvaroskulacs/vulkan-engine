#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "swap_chain.h"

namespace engine
{

class CommandBuffer
{
public:
    vk::CommandBuffer* GetBufferPtr(size_t idx);
    std::vector<vk::CommandBuffer> GetBuffers();

    explicit CommandBuffer(std::shared_ptr<Device> device, std::shared_ptr<SwapChain> swapChain);
    ~CommandBuffer();

    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

private:
    void createCommandPool();
    void createCommandBuffers();

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;

    vk::CommandPool m_commandPool;
    std::vector<vk::CommandBuffer> m_commandBuffers;
};

} // namespace engine