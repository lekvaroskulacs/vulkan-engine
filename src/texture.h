#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "command_buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace engine
{

class Texture
{
public:
    vk::ImageView GetImageView() const
    {
        return m_textureImageView;
    }

    vk::Sampler GetSampler() const
    {
        return m_textureSampler;
    }

    //TODO: also make filepath a parameter
    explicit Texture(std::shared_ptr<Device> device,
                     std::shared_ptr<SwapChain> swapChain,
                     std::shared_ptr<CommandBuffer> commandBuffer)
        : m_device(device)
        , m_swapChain(swapChain)
        , m_commandBuffer(commandBuffer)
    {
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
    }

    ~Texture()
    {
        m_device->GetDevice().destroySampler(m_textureSampler, nullptr);
        m_device->GetDevice().destroyImageView(m_textureImageView, nullptr);

        m_device->GetDevice().destroyImage(m_textureImage, nullptr);
        m_device->GetDevice().freeMemory(m_textureImageMemory, nullptr);
    }

private:
    void createTextureSampler()
    {
        vk::PhysicalDeviceProperties properties{};
        m_device->GetPhysicalDevice().getProperties(&properties);

        vk::SamplerCreateInfo samplerInfo{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eRepeat,
            .addressModeV = vk::SamplerAddressMode::eRepeat,
            .addressModeW = vk::SamplerAddressMode::eRepeat,
            .mipLodBias = 0.0f,
            .anisotropyEnable = vk::True,
            .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
            .compareEnable = vk::False,
            .compareOp = vk::CompareOp::eAlways,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = vk::BorderColor::eIntOpaqueBlack,
            .unnormalizedCoordinates = vk::False,
        };

        if(m_device->GetDevice().createSampler(&samplerInfo, nullptr, &m_textureSampler) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void createTextureImageView()
    {
        m_textureImageView = m_swapChain->createImageView(
            m_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
    }

    void createTextureImage()
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels =
            stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        vk::DeviceSize imageSize = texWidth * texHeight * 4;

        if(!pixels)
        {
            throw std::runtime_error("Failed to load texture image!");
        }

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;
        m_device->createBuffer(imageSize,
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent,
                               stagingBuffer,
                               stagingBufferMemory);

        void* data;
        [[maybe_unused]] auto ignored =
            m_device->GetDevice().mapMemory(stagingBufferMemory, 0, imageSize, {}, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        m_device->GetDevice().unmapMemory(stagingBufferMemory);

        stbi_image_free(pixels);

        m_swapChain->createImage(texWidth,
                                 texHeight,
                                 vk::Format::eR8G8B8A8Srgb,
                                 vk::ImageTiling::eOptimal,
                                 vk::ImageUsageFlagBits::eTransferDst |
                                     vk::ImageUsageFlagBits::eSampled,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 m_textureImage,
                                 m_textureImageMemory);

        transitionImageLayout(m_textureImage,
                              vk::Format::eR8G8B8A8Srgb,
                              vk::ImageLayout::eUndefined,
                              vk::ImageLayout::eTransferDstOptimal);

        copyBufferToImage(stagingBuffer,
                          m_textureImage,
                          static_cast<uint32_t>(texWidth),
                          static_cast<uint32_t>(texHeight));
        transitionImageLayout(m_textureImage,
                              vk::Format::eR8G8B8A8Srgb,
                              vk::ImageLayout::eTransferDstOptimal,
                              vk::ImageLayout::eShaderReadOnlyOptimal);

        m_device->GetDevice().destroyBuffer(stagingBuffer, nullptr);
        m_device->GetDevice().freeMemory(stagingBufferMemory, nullptr);
    }

    void transitionImageLayout(vk::Image image,
                               vk::Format format,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout)
    {
        vk::CommandBuffer commandBuffer = m_commandBuffer->beginSingleTimeCommands();

        vk::ImageSubresourceRange subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
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

        if(oldLayout == vk::ImageLayout::eUndefined &&
           newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if(oldLayout == vk::ImageLayout::eTransferDstOptimal &&
                newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        commandBuffer.pipelineBarrier(
            sourceStage, destinationStage, {}, 0, nullptr, 0, nullptr, 1, &barrier);

        m_commandBuffer->endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
    {
        vk::CommandBuffer commandBuffer = m_commandBuffer->beginSingleTimeCommands();

        vk::ImageSubresourceLayers subresource{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        vk::BufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = subresource,
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1},
        };

        commandBuffer.copyBufferToImage(
            buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

        m_commandBuffer->endSingleTimeCommands(commandBuffer);
    }

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;
    std::shared_ptr<CommandBuffer> m_commandBuffer;

    vk::Image m_textureImage;
    vk::DeviceMemory m_textureImageMemory;
    vk::ImageView m_textureImageView;
    vk::Sampler m_textureSampler;
};

} // namespace engine
