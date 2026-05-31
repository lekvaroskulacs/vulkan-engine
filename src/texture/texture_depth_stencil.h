#pragma once

#include "texture.h"

namespace engine
{

struct TextureDepthStencilParams
{
    uint32_t m_width, m_height;
    vk::Format m_depthFormat;
};

class TextureDepthStencil : public Texture
{
public:
    explicit TextureDepthStencil(std::shared_ptr<Device> device,
                                 std::shared_ptr<CommandBuffer> commandBuffer,
                                 TextureDepthStencilParams&& params)
        : Texture(device, commandBuffer)
    {
        m_device->createImage(params.m_width,
                              params.m_height,
                              params.m_depthFormat,
                              vk::ImageTiling::eOptimal,
                              vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                              vk::MemoryPropertyFlagBits::eDeviceLocal,
                              m_textureImage,
                              m_textureImageAllocation);
        m_textureImageView = m_device->createImageView(m_textureImage, params.m_depthFormat, vk::ImageAspectFlagBits::eDepth);
    }

    virtual ~TextureDepthStencil() override
    {
        destroyTexture();
    }
};

} // namespace engine
