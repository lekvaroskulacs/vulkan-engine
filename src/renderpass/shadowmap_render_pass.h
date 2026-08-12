#pragma once

#include "render_pass.h"

namespace engine
{

/// Planar (single-direction) shadow map pass. Not currently wired up in the engine; kept for symmetry
/// with PipelineShadowMap and as a starting point for directional/spot light shadows.
class ShadowMapRenderPass : public RenderPass
{
public:
    vk::Extent2D GetExtent() const override;
    vk::Framebuffer GetFrameBuffer(uint32_t instance, uint32_t imageIndex) const override;
    std::vector<vk::ClearValue> GetClearValues() const override;
    Texture* GetRenderTarget(uint32_t index = 0) const override;

    explicit ShadowMapRenderPass(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer);
    ~ShadowMapRenderPass() override;

    constexpr static uint32_t m_shadowMapSize = 2024;

private:
    void createRenderPass();
    void createRenderTarget();
    void createFrameBuffer();

    std::shared_ptr<CommandBuffer> m_commandBuffer;

    std::unique_ptr<Texture> m_renderTarget;
    vk::Framebuffer m_framebuffer;
};

} // namespace engine
