#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "pipeline.h"

namespace engine
{

class CommandBuffer
{
public:
    VkCommandBuffer* GetBufferPtr(size_t idx)
    {
        return &m_commandBuffers[idx];
    }

    std::vector<VkCommandBuffer> GetBuffers()
    {
        return m_commandBuffers;
    }

    explicit CommandBuffer(std::shared_ptr<Device> device, std::shared_ptr<SwapChain> swapChain)
        : m_device(device)
        , m_swapChain(swapChain)
    {
        createCommandPool();
        createCommandBuffers();
    }

    ~CommandBuffer()
    {
        vkDestroyCommandPool(m_device->GetDevice(), m_commandPool, nullptr);
    }

    VkCommandBuffer beginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_device->GetDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_device->GetGraphicsQueue());

        vkFreeCommandBuffers(m_device->GetDevice(), m_commandPool, 1, &commandBuffer);
    }

private:
    void createCommandPool()
    {
        engine::utils::QueueFamilyIndices queueFamilyIndices =
            engine::utils::QueueFamilyIndices::findQueueFamilies(m_device->GetPhysicalDevice(),
                                                                 m_device->GetSurface());

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value();

        if(vkCreateCommandPool(m_device->GetDevice(), &poolInfo, nullptr, &m_commandPool) !=
           VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    void createCommandBuffers()
    {
        m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

        if(vkAllocateCommandBuffers(m_device->GetDevice(), &allocInfo, m_commandBuffers.data()) !=
           VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;

    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
};

} // namespace engine