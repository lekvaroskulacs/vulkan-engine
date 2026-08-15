#include <engine/renderpass/shadowmap_render_pass.h>
#include <engine/texture/texture_framebuffer.h>
#include <array>
#include <stdexcept>

namespace engine
{

vk::Extent2D ShadowMapRenderPass::GetExtent() const
{
    return {m_shadowMapSize, m_shadowMapSize};
}

vk::Framebuffer ShadowMapRenderPass::GetFrameBuffer(uint32_t instance, uint32_t imageIndex) const
{
    return m_framebuffer;
}

std::vector<vk::ClearValue> ShadowMapRenderPass::GetClearValues() const
{
    vk::ClearValue depthClear{.depthStencil = vk::ClearDepthStencilValue{1.0f, 0}};
    return {depthClear};
}

Texture* ShadowMapRenderPass::GetRenderTarget(uint32_t index) const
{
    return m_renderTarget.get();
}

ShadowMapRenderPass::ShadowMapRenderPass(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer)
    : RenderPass(device)
    , m_commandBuffer{commandBuffer}
{
    createRenderTarget();
    createRenderPass();
    createFrameBuffer();
}

ShadowMapRenderPass::~ShadowMapRenderPass()
{
    m_device->GetDevice().destroyFramebuffer(m_framebuffer, nullptr);
}

void ShadowMapRenderPass::createRenderTarget()
{
    TextureFramebufferParams params{.m_width = m_shadowMapSize,
                                    .m_height = m_shadowMapSize,
                                    .m_usageFlags = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
                                    .m_aspectFlags = vk::ImageAspectFlagBits::eDepth,
                                    .m_format = m_device->findDepthFormat()};

    m_renderTarget = std::make_unique<TextureFramebuffer>(m_device, m_commandBuffer, std::move(params));
}

void ShadowMapRenderPass::createRenderPass()
{
    vk::AttachmentDescription depthAttachment{.format = m_device->findDepthFormat(),
                                              .samples = vk::SampleCountFlagBits::e1,
                                              .loadOp = vk::AttachmentLoadOp::eClear,
                                              .storeOp = vk::AttachmentStoreOp::eStore,
                                              .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                              .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                                              .initialLayout = vk::ImageLayout::eUndefined,
                                              .finalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal};

    vk::AttachmentReference depthAttachmentRef{.attachment = 0, .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 0,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    std::array<vk::SubpassDependency, 2> dependencies{};

    dependencies[0] = {
        .srcSubpass = vk::SubpassExternal,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eFragmentShader,
        .dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eShaderRead,
        .dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dependencyFlags = vk::DependencyFlagBits::eByRegion,
    };

    dependencies[1] = {
        .srcSubpass = 0,
        .dstSubpass = vk::SubpassExternal,
        .srcStageMask = vk::PipelineStageFlagBits::eLateFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eFragmentShader,
        .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .dependencyFlags = vk::DependencyFlagBits::eByRegion,
    };

    vk::RenderPassCreateInfo renderPassInfo{.attachmentCount = 1,
                                            .pAttachments = &depthAttachment,
                                            .subpassCount = 1,
                                            .pSubpasses = &subpass,
                                            .dependencyCount = static_cast<uint32_t>(dependencies.size()),
                                            .pDependencies = dependencies.data()};

    if(m_device->GetDevice().createRenderPass(&renderPassInfo, nullptr, &m_renderPass) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void ShadowMapRenderPass::createFrameBuffer()
{
    vk::FramebufferCreateInfo frameBufferInfo{.renderPass = m_renderPass,
                                              .attachmentCount = 1,
                                              .pAttachments = m_renderTarget->GetImageView_ptr(),
                                              .width = m_shadowMapSize,
                                              .height = m_shadowMapSize,
                                              .layers = 1};

    if(m_device->GetDevice().createFramebuffer(&frameBufferInfo, nullptr, &m_framebuffer) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create framebuffer!");
    }
}

} // namespace engine
