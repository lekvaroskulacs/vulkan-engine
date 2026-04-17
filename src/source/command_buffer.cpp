#include "../include/command_buffer.h"

namespace engine
{

vk::CommandBuffer* CommandBuffer::GetBufferPtr(size_t idx)
{
    return &m_commandBuffers[idx];
}

std::vector<vk::CommandBuffer> CommandBuffer::GetBuffers()
{
    return m_commandBuffers;
}

CommandBuffer::CommandBuffer(std::shared_ptr<Device> device, std::shared_ptr<SwapChain> swapChain)
    : m_device(device)
    , m_swapChain(swapChain)
{
    createCommandPool();
    createCommandBuffers();
}

CommandBuffer::~CommandBuffer()
{
    m_device->GetDevice().destroyCommandPool(m_commandPool, nullptr);
}

vk::CommandBuffer CommandBuffer::beginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };

    vk::CommandBuffer commandBuffer;
    [[maybe_unused]] auto result = m_device->GetDevice().allocateCommandBuffers(&allocInfo, &commandBuffer);

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    result = commandBuffer.begin(&beginInfo);

    return commandBuffer;
}

void CommandBuffer::endSingleTimeCommands(vk::CommandBuffer commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &commandBuffer};

    [[maybe_unused]] auto result = m_device->GetGraphicsQueue().submit(1, &submitInfo, VK_NULL_HANDLE);
    m_device->GetGraphicsQueue().waitIdle();

    m_device->GetDevice().freeCommandBuffers(m_commandPool, 1, &commandBuffer);
}

void CommandBuffer::createCommandPool()
{
    engine::utils::QueueFamilyIndices queueFamilyIndices =
        engine::utils::QueueFamilyIndices::findQueueFamilies(m_device->GetPhysicalDevice(), m_device->GetSurface());

    vk::CommandPoolCreateInfo poolInfo{.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                       .queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value()};

    if(m_device->GetDevice().createCommandPool(&poolInfo, nullptr, &m_commandPool) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void CommandBuffer::createCommandBuffers()
{
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size()),
    };

    if(m_device->GetDevice().allocateCommandBuffers(&allocInfo, m_commandBuffers.data()) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

} // namespace engine
