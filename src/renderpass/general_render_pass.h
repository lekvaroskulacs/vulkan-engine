#pragma once

#include "../include/swap_chain.h"
#include "render_pass.h"

namespace engine
{

/// The main color+depth pass that draws into the swap chain's presentable images.
class GeneralRenderPass : public RenderPass
{
public:
    vk::Extent2D GetExtent() const override;
    vk::Framebuffer GetFrameBuffer(uint32_t instance, uint32_t imageIndex) const override;
    std::vector<vk::ClearValue> GetClearValues() const override;

    explicit GeneralRenderPass(std::shared_ptr<Device> device, std::shared_ptr<SwapChain> swapChain);
    ~GeneralRenderPass() override;

    void recreate() override;

private:
    void createRenderPass();
    void createDepthResources();
    void createFrameBuffers();
    void cleanupFrameBufferResources();

    std::shared_ptr<SwapChain> m_swapChain;

    vk::Image m_depthImage;
    VmaAllocation m_depthImageAllocation;
    vk::ImageView m_depthImageView;

    std::vector<vk::Framebuffer> m_framebuffers;
};

} // namespace engine
