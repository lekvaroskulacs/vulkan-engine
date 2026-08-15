#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include <engine/command_buffer/command_buffer.h>

#include <stb_image.h>
#include <variant>

namespace engine
{

class Texture
{
public:
    vk::ImageView GetImageView() const
    {
        return m_textureImageView;
    }

    const vk::ImageView* GetImageView_ptr() const
    {
        return &m_textureImageView;
    }

    virtual vk::Sampler GetSampler() const
    {
        return m_textureSampler;
    }

    vk::Image GetImage() const
    {
        return m_textureImage;
    }

    virtual vk::ImageAspectFlags GetAspectFlags() const
    {
        return vk::ImageAspectFlagBits::eColor;
    }

    explicit Texture(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer)
        : m_device(device)
        , m_commandBuffer(commandBuffer)
    { }

    // Allocations only happen in derived classes, so destroying resources here could potentially cause segfault
    virtual ~Texture() { }

    void transitionImageLayout(vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout,
                               uint32_t barrierLayerCount = 1,
                               vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor)
    {
        transitionImageLayout(m_textureImage, {}, oldLayout, newLayout, barrierLayerCount, aspectFlags);
    }

protected:
    void transitionImageLayout(vk::Image image,
                               vk::Format format,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout,
                               uint32_t barrierLayerCount = 1,
                               vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor)
    {
        vk::CommandBuffer commandBuffer = m_commandBuffer->beginSingleTimeCommands();

        vk::ImageSubresourceRange subresourceRange{
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = barrierLayerCount,
        };
        vk::ImageMemoryBarrier barrier{
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = image,
            .subresourceRange = subresourceRange,
        };

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if(oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if(oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else if(oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else if(oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthAttachmentOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eAllCommands;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, 0, nullptr, 0, nullptr, 1, &barrier);

        m_commandBuffer->endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, uint32_t layerCount = 1)
    {
        vk::CommandBuffer commandBuffer = m_commandBuffer->beginSingleTimeCommands();

        vk::ImageSubresourceLayers subresource{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = layerCount,
        };
        vk::BufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = subresource,
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1},
        };

        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

        m_commandBuffer->endSingleTimeCommands(commandBuffer);
    }

    void destroyTexture()
    {
        m_device->GetDevice().destroySampler(m_textureSampler, nullptr);
        m_device->GetDevice().destroyImageView(m_textureImageView, nullptr);

        m_device->destroyImage(m_textureImage, m_textureImageAllocation);
    }

protected:
    std::shared_ptr<Device> m_device;
    std::shared_ptr<CommandBuffer> m_commandBuffer;

    vk::Image m_textureImage;
    VmaAllocation m_textureImageAllocation;
    vk::ImageView m_textureImageView;
    vk::Sampler m_textureSampler;
};

} // namespace engine
