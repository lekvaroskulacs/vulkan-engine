#pragma once

#include "texture.h"

namespace engine
{

struct TextureViewOnlyParams
{
    Texture* m_reference;
    uint32_t m_baseArrayLayer = 0;
};

class TextureViewOnly : public Texture
{
public:
    virtual vk::Sampler GetSampler() const
    {
        throw std::runtime_error("View only texture doesn't have a sampler!");
    }

    explicit TextureViewOnly(std::shared_ptr<Device> device,
                             std::shared_ptr<CommandBuffer> commandBuffer,
                             TextureViewOnlyParams&& params)
        : Texture(device, commandBuffer)
    {
        vk::ImageSubresourceRange range{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = params.m_baseArrayLayer,
            .layerCount = 1,
        };

        vk::ImageViewCreateInfo info{
            .image = params.m_reference->GetImage(),
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

    virtual ~TextureViewOnly() override
    {
        m_device->GetDevice().destroyImageView(m_textureImageView, nullptr);
    }
};

} // namespace engine
