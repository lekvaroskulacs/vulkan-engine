#pragma once

#include "texture.h"

namespace engine
{

struct Texture2DParams
{
    std::string m_filepath;
};

class Texture2D : public Texture
{
public:
    explicit Texture2D(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, Texture2DParams&& params)
        : Texture(device, commandBuffer)
        , m_params(std::move(params))
    {
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
    }

    virtual ~Texture2D() override
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
        m_textureImageView = m_device->createImageView(m_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
    }

    void createTextureImage()
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(m_params.m_filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
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

    Texture2DParams m_params;
};

} // namespace engine
