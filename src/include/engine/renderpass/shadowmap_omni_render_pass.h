#pragma once

#include <engine/renderpass/render_pass.h>

namespace engine
{

/// Omnidirectional (cubemap) point-light shadow pass: one render pass invocation per cube face.
class OmniShadowMapRenderPass : public RenderPass
{
public:
    vk::Extent2D GetExtent() const override;
    uint32_t GetInstanceCount() const override;
    vk::Framebuffer GetFrameBuffer(uint32_t instance, uint32_t imageIndex) const override;
    std::vector<vk::ClearValue> GetClearValues() const override;
    Texture* GetRenderTarget(uint32_t index = 0) const override;
    std::optional<PushConstantData> GetPushConstants(uint32_t instance, const DrawFrameData& frame) const override;

    explicit OmniShadowMapRenderPass(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer);
    ~OmniShadowMapRenderPass() override;

    constexpr static uint32_t m_shadowMapSize = 2024;

private:
    void createRenderPass();
    void createRenderTargets();
    void createFrameBuffers();

    std::shared_ptr<CommandBuffer> m_commandBuffer;

    std::vector<std::unique_ptr<Texture>> m_renderTargets;
    std::unique_ptr<Texture> m_shadowDepthStencil;
    std::vector<vk::Framebuffer> m_framebuffers;
};

} // namespace engine
