#pragma once

#include "texture.h"

namespace engine
{

struct TextureCubeParams
{
    std::array<std::string, 6> m_filepaths;
};

class TextureCube : public Texture
{
public:
    explicit TextureCube(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, TextureCubeParams&& params)
        : Texture(device, commandBuffer)
        , m_params(std::move(params))
    {
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
    }

    virtual ~TextureCube() override
    {
        destroyTexture();
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
        m_textureImageView = m_device->createImageView(
            m_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, 6, vk::ImageViewType::eCube);
    }

    void createTextureImage()
    {
        std::vector<stbi_uc*> pixels;
        pixels.resize(6);
        int texWidth, texHeight, texChannels;
        for(int i = 0; i < 6; ++i)
        {
            pixels[i] = stbi_load(m_params.m_filepaths[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
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

    TextureCubeParams m_params;
};

} // namespace engine
