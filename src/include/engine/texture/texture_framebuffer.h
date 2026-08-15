#pragma once

#include <engine/texture/texture.h>

namespace engine
{

struct TextureFramebufferParams
{
    uint32_t m_width, m_height;
    vk::ImageUsageFlags m_usageFlags;
    vk::ImageAspectFlags m_aspectFlags;
    vk::Format m_format;
    uint32_t m_baseArrayLayer = 0;
    bool m_isCube = false;
};

class TextureFramebuffer : public Texture
{
public:
    explicit TextureFramebuffer(std::shared_ptr<Device> device,
                                std::shared_ptr<CommandBuffer> commandBuffer,
                                TextureFramebufferParams&& params)
        : Texture(device, commandBuffer)
        , m_params(std::move(params))
    {
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
    }

    vk::ImageAspectFlags GetAspectFlags() const override
    {
        return m_params.m_aspectFlags;
    }

    virtual ~TextureFramebuffer() override
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

    void createTextureImageView()
    {
        uint32_t layers = m_params.m_isCube ? 6 : 1;
        vk::ImageViewType viewType = m_params.m_isCube ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;

        m_textureImageView = m_device->createImageView(m_textureImage, m_params.m_format, m_params.m_aspectFlags, layers, viewType);
    }

    void createTextureImage()
    {
        vk::DeviceSize imageSize = m_params.m_width * m_params.m_height;
        uint32_t texWidth = m_params.m_width;
        uint32_t texHeight = m_params.m_height;
        uint32_t arrayLayers = m_params.m_isCube ? 6 : 1;
        vk::ImageCreateFlags createFlags{};

        if(m_params.m_isCube)
        {
            imageSize *= 6;
            createFlags = vk::ImageCreateFlagBits::eCubeCompatible;
        }

        m_device->createImage(texWidth,
                              texHeight,
                              m_params.m_format,
                              vk::ImageTiling::eOptimal,
                              m_params.m_usageFlags,
                              vk::MemoryPropertyFlagBits::eDeviceLocal,
                              m_textureImage,
                              m_textureImageAllocation,
                              createFlags,
                              arrayLayers);
    }

    TextureFramebufferParams m_params;
};

} // namespace engine
