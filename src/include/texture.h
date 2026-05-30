#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "command_buffer.h"

#include <stb_image.h>
#include <variant>

namespace engine
{

class Texture;

// TODO: inheritance instead of variant
struct Texture2DParams
{
    std::string m_filepath;
};

struct TextureCubeParams
{
    std::array<std::string, 6> m_filepaths;
};

struct TextureFramebufferParams
{
    uint32_t m_width, m_height;
    vk::ImageUsageFlags m_usageFlags;
    vk::ImageAspectFlags m_aspectFlags;
    vk::Format m_format;
    uint32_t m_baseArrayLayer = 0;
    bool m_isCube = false;
};

struct TextureDepthStencilParams
{
    uint32_t m_width, m_height;
    vk::Format m_depthFormat;
};

struct TextureViewOnlyParams
{
    Texture* m_reference;
    uint32_t m_baseArrayLayer = 0;
};

using TextureParams =
    std::variant<Texture2DParams, TextureCubeParams, TextureFramebufferParams, TextureDepthStencilParams, TextureViewOnlyParams>;

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

    // TODO: throw errors if called with viewonly (inheritance makes this cleaner)
    vk::Sampler GetSampler() const
    {
        return m_textureSampler;
    }

    vk::Image GetImage() const
    {
        return m_textureImage;
    }

    vk::ImageAspectFlags GetAspectFlags() const
    {
        if(auto* params = std::get_if<TextureFramebufferParams>(&m_params))
        {
            return params->m_aspectFlags;
        }

        return vk::ImageAspectFlagBits::eColor;
    }

    explicit Texture(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, TextureParams&& params)
        : m_device(device)
        , m_commandBuffer(commandBuffer)
        , m_params(std::move(params))
    {
        if(std::holds_alternative<Texture2DParams>(params))
        {
            createTextureImage();
            createTextureImageView();
            createTextureSampler();
        }
        else if(std::holds_alternative<TextureCubeParams>(params))
        {
            createTextureCubeImage();
            createTextureCubeImageView();
            createTextureSampler();
        }
        else if(std::holds_alternative<TextureFramebufferParams>(params))
        {
            createTextureFramebufferImage();
            createTextureFramebufferImageView();
            createTextureShadowSampler();
        }
        else if(std::holds_alternative<TextureDepthStencilParams>(params))
        {
            auto depth_params = std::get<TextureDepthStencilParams>(params);
            m_device->createImage(depth_params.m_width,
                                  depth_params.m_height,
                                  depth_params.m_depthFormat,
                                  vk::ImageTiling::eOptimal,
                                  vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                  vk::MemoryPropertyFlagBits::eDeviceLocal,
                                  m_textureImage,
                                  m_textureImageAllocation);
            m_textureImageView =
                m_device->createImageView(m_textureImage, depth_params.m_depthFormat, vk::ImageAspectFlagBits::eDepth);
        }
        else if(auto* view_only_params = std::get_if<TextureViewOnlyParams>(&params))
        {
            vk::ImageSubresourceRange range{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = view_only_params->m_baseArrayLayer,
                .layerCount = 1,
            };

            vk::ImageViewCreateInfo info{
                .image = view_only_params->m_reference->GetImage(),
                .viewType = vk::ImageViewType::e2D,
                .format = vk::Format::eR32Sfloat,
                .components = {vk::ComponentSwizzle::eR},
                .subresourceRange = range,
            };

            if(m_device->GetDevice().createImageView(&info, nullptr, &m_textureImageView) != vk::Result::eSuccess)
            {
                throw std::runtime_error("failed to create image view!");
            }
        }
    }

    ~Texture()
    {
        m_device->GetDevice().destroySampler(m_textureSampler, nullptr);
        m_device->GetDevice().destroyImageView(m_textureImageView, nullptr);

        m_device->destroyImage(m_textureImage, m_textureImageAllocation);
    }

    void transitionImageLayout(vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout,
                               uint32_t barrierLayerCount = 1,
                               vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor)
    {
        transitionImageLayout(m_textureImage, {}, oldLayout, newLayout, barrierLayerCount, aspectFlags);
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
            .maxLod = 1.0f,
            .borderColor = vk::BorderColor::eIntOpaqueWhite,
            .unnormalizedCoordinates = vk::False,
        };

        if(m_device->GetDevice().createSampler(&samplerInfo, nullptr, &m_textureSampler) != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void createTextureImageView()
    {
        m_textureImageView = m_device->createImageView(m_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
    }

    void createTextureImage()
    {
        auto* params = std::get_if<Texture2DParams>(&m_params);
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(params->m_filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        vk::DeviceSize imageSize = texWidth * texHeight * 4;

        if(!pixels)
        {
            throw std::runtime_error("Failed to load texture image!");
        }

        vk::Buffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        m_device->createBuffer(imageSize,
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                               stagingBuffer,
                               stagingBufferAllocation);

        m_device->copyMemoryToAllocation(pixels, stagingBufferAllocation, imageSize);

        stbi_image_free(pixels);

        m_device->createImage(texWidth,
                              texHeight,
                              vk::Format::eR8G8B8A8Srgb,
                              vk::ImageTiling::eOptimal,
                              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                              vk::MemoryPropertyFlagBits::eDeviceLocal,
                              m_textureImage,
                              m_textureImageAllocation);

        transitionImageLayout(
            m_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        copyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(
            m_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        m_device->destroyBuffer(stagingBuffer, stagingBufferAllocation);
    }

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

    void createTextureCubeImage()
    {
        auto* params = std::get_if<TextureCubeParams>(&m_params);

        std::vector<stbi_uc*> pixels;
        pixels.resize(6);
        int texWidth, texHeight, texChannels;
        for(int i = 0; i < 6; ++i)
        {
            pixels[i] = stbi_load(params->m_filepaths[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        }
        vk::DeviceSize layerSize = texWidth * texHeight * 4;
        vk::DeviceSize imageSize = layerSize * 6;

        bool anyFailed = false;
        for(int i = 0; i < 6; ++i)
        {
            if(!pixels[i])
            {
                anyFailed = true;
                break;
            }
        }
        if(anyFailed)
        {
            throw std::runtime_error("Failed to load texture image!");
        }

        vk::Buffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        m_device->createBuffer(imageSize,
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                               stagingBuffer,
                               stagingBufferAllocation);

        for(int i = 0; i < 6; ++i)
        {
            m_device->copyMemoryToAllocation(pixels[i], stagingBufferAllocation, layerSize, i * layerSize);
        }

        for(auto element : pixels)
        {
            stbi_image_free(element);
        }

        m_device->createImage(texWidth,
                              texHeight,
                              vk::Format::eR8G8B8A8Srgb,
                              vk::ImageTiling::eOptimal,
                              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                              vk::MemoryPropertyFlagBits::eDeviceLocal,
                              m_textureImage,
                              m_textureImageAllocation,
                              {vk::ImageCreateFlagBits::eCubeCompatible},
                              6);

        transitionImageLayout(
            m_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 6);

        copyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 6);
        transitionImageLayout(m_textureImage,
                              vk::Format::eR8G8B8A8Srgb,
                              vk::ImageLayout::eTransferDstOptimal,
                              vk::ImageLayout::eShaderReadOnlyOptimal,
                              6);

        m_device->destroyBuffer(stagingBuffer, stagingBufferAllocation);
    }

    void createTextureCubeImageView()
    {
        m_textureImageView = m_device->createImageView(
            m_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, 6, vk::ImageViewType::eCube);
    }

    void copyBufferToImageCube(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, uint32_t layerCount = 1)
    {
        vk::CommandBuffer commandBuffer = m_commandBuffer->beginSingleTimeCommands();

        std::vector<vk::BufferImageCopy> regions(6);
        for(uint32_t i = 0; i < 6; ++i)
        {
            vk::ImageSubresourceLayers subresource{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = i,
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
        }
        commandBuffer.copyBufferToImage(
            buffer, image, vk::ImageLayout::eTransferDstOptimal, static_cast<uint32_t>(regions.size()), regions.data());

        m_commandBuffer->endSingleTimeCommands(commandBuffer);
    }

    void createTextureFramebufferImage()
    {
        auto* params = std::get_if<TextureFramebufferParams>(&m_params);

        vk::DeviceSize imageSize = params->m_width * params->m_height;
        uint32_t texWidth = params->m_width;
        uint32_t texHeight = params->m_height;
        uint32_t arrayLayers = params->m_isCube ? 6 : 1;
        vk::ImageCreateFlags createFlags{};

        if(params->m_isCube)
        {
            imageSize *= 6;
            createFlags = vk::ImageCreateFlagBits::eCubeCompatible;
        }

        m_device->createImage(texWidth,
                              texHeight,
                              params->m_format,
                              vk::ImageTiling::eOptimal,
                              params->m_usageFlags,
                              vk::MemoryPropertyFlagBits::eDeviceLocal,
                              m_textureImage,
                              m_textureImageAllocation,
                              createFlags,
                              arrayLayers);
    }

    void createTextureFramebufferImageView()
    {
        auto* params = std::get_if<TextureFramebufferParams>(&m_params);
        uint32_t layers = params->m_isCube ? 6 : 1;
        vk::ImageViewType viewType = params->m_isCube ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;

        m_textureImageView = m_device->createImageView(m_textureImage, params->m_format, params->m_aspectFlags, layers, viewType);
    }

    void createTextureShadowSampler()
    {
        vk::PhysicalDeviceProperties properties{};
        m_device->GetPhysicalDevice().getProperties(&properties);

        vk::SamplerCreateInfo samplerInfo{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eClampToBorder,
            .addressModeV = vk::SamplerAddressMode::eClampToBorder,
            .addressModeW = vk::SamplerAddressMode::eClampToBorder,
            .mipLodBias = 0.0f,
            .anisotropyEnable = vk::True,
            .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
            .compareEnable = vk::False,
            .compareOp = vk::CompareOp::eAlways,
            .minLod = 0.0f,
            .maxLod = 1.0f,
            .borderColor = vk::BorderColor::eIntOpaqueWhite,
            .unnormalizedCoordinates = vk::False,
        };

        if(m_device->GetDevice().createSampler(&samplerInfo, nullptr, &m_textureSampler) != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    std::shared_ptr<Device> m_device;
    std::shared_ptr<CommandBuffer> m_commandBuffer;
    TextureParams m_params;

    vk::Image m_textureImage;
    VmaAllocation m_textureImageAllocation;
    vk::ImageView m_textureImageView;
    vk::Sampler m_textureSampler;
};

} // namespace engine
