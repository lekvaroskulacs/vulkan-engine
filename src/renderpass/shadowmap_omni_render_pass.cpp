#include <engine/renderpass/shadowmap_omni_render_pass.h>
#include <engine/renderer/renderer.h>
#include <engine/texture/texture_depth_stencil.h>
#include <engine/texture/texture_framebuffer.h>
#include <engine/texture/texture_view_only.h>
#include <array>
#include <stdexcept>

namespace engine
{

vk::Extent2D OmniShadowMapRenderPass::GetExtent() const
{
    return {m_shadowMapSize, m_shadowMapSize};
}

uint32_t OmniShadowMapRenderPass::GetInstanceCount() const
{
    return 6;
}

vk::Framebuffer OmniShadowMapRenderPass::GetFrameBuffer(uint32_t instance, uint32_t imageIndex) const
{
    return m_framebuffers[instance];
}

std::vector<vk::ClearValue> OmniShadowMapRenderPass::GetClearValues() const
{
    vk::ClearValue colorClear{.color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f})};
    vk::ClearValue depthClear{.depthStencil = vk::ClearDepthStencilValue{1.0f, 0}};
    return {colorClear, depthClear};
}

Texture* OmniShadowMapRenderPass::GetRenderTarget(uint32_t index) const
{
    return m_renderTargets.at(index).get();
}

std::optional<PushConstantData> OmniShadowMapRenderPass::GetPushConstants(uint32_t instance, const PerMeshRenderData& frame) const
{
    auto lightPos = frame.m_shadow_light_position;
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    switch(instance)
    {
    case 0: // POSITIVE_X
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0));
        break;
    case 1: // NEGATIVE_X
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0));
        break;
    case 2: // POSITIVE_Y
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
        break;
    case 3: // NEGATIVE_Y
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));
        break;
    case 4: // POSITIVE_Z
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0));
        break;
    case 5: // NEGATIVE_Z
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0));
        break;
    }

    return PushConstantData{.m_stage = vk::ShaderStageFlagBits::eVertex, .m_matrix = viewMatrix};
}

OmniShadowMapRenderPass::OmniShadowMapRenderPass(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer)
    : RenderPass(device)
    , m_commandBuffer{commandBuffer}
{
    createRenderTargets();
    createRenderPass();
    createFrameBuffers();
}

OmniShadowMapRenderPass::~OmniShadowMapRenderPass()
{
    for(auto& framebuffer : m_framebuffers)
    {
        m_device->GetDevice().destroyFramebuffer(framebuffer, nullptr);
    }
}

void OmniShadowMapRenderPass::createRenderTargets()
{
    TextureFramebufferParams params{.m_width = m_shadowMapSize,
                                    .m_height = m_shadowMapSize,
                                    .m_usageFlags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                    .m_aspectFlags = vk::ImageAspectFlagBits::eColor,
                                    .m_format = vk::Format::eR32Sfloat,
                                    .m_isCube = true};

    auto tex = std::make_unique<TextureFramebuffer>(m_device, m_commandBuffer, std::move(params));
    tex->transitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, 6);
    m_renderTargets.push_back(std::move(tex));

    //also have 6 image views for all cube faces
    for(uint32_t i = 0; i < 6; ++i)
    {
        TextureViewOnlyParams face{.m_reference = m_renderTargets.at(0).get(), .m_baseArrayLayer = i};

        auto faceView = std::make_unique<TextureViewOnly>(m_device, m_commandBuffer, std::move(face));
        m_renderTargets.push_back(std::move(faceView));
    }

    m_shadowDepthStencil = std::make_unique<TextureDepthStencil>(
        m_device,
        m_commandBuffer,
        std::move(TextureDepthStencilParams{
            .m_width = m_shadowMapSize, .m_height = m_shadowMapSize, .m_depthFormat = m_device->findDepthFormat()}));
    m_shadowDepthStencil->transitionImageLayout(
        vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal, 1, vk::ImageAspectFlagBits::eDepth);
}

void OmniShadowMapRenderPass::createRenderPass()
{
    vk::AttachmentDescription colorAttachment{.format = vk::Format::eR32Sfloat,
                                              .samples = vk::SampleCountFlagBits::e1,
                                              .loadOp = vk::AttachmentLoadOp::eClear,
                                              .storeOp = vk::AttachmentStoreOp::eStore,
                                              .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                              .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                                              .initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                                              .finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    vk::AttachmentReference colorAttachmentRef{.attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

    vk::AttachmentDescription depthAttachment{.format = m_device->findDepthFormat(),
                                              .samples = vk::SampleCountFlagBits::e1,
                                              .loadOp = vk::AttachmentLoadOp::eClear,
                                              .storeOp = vk::AttachmentStoreOp::eStore,
                                              .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                              .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                                              .initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                              .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

    vk::AttachmentReference depthAttachmentRef{.attachment = 1, .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    vk::RenderPassCreateInfo renderPassInfo{.attachmentCount = static_cast<uint32_t>(attachments.size()),
                                            .pAttachments = attachments.data(),
                                            .subpassCount = 1,
                                            .pSubpasses = &subpass,
                                            .dependencyCount = 0,
                                            .pDependencies = nullptr};

    if(m_device->GetDevice().createRenderPass(&renderPassInfo, nullptr, &m_renderPass) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void OmniShadowMapRenderPass::createFrameBuffers()
{
    m_framebuffers.resize(6);

    vk::ImageView attachments[2];
    attachments[1] = m_shadowDepthStencil->GetImageView();
    for(int i = 0; i < 6; ++i)
    {
        attachments[0] = m_renderTargets.at(i + 1)->GetImageView();
        vk::FramebufferCreateInfo frameBufferInfo{.renderPass = m_renderPass,
                                                  .attachmentCount = 2,
                                                  .pAttachments = attachments,
                                                  .width = m_shadowMapSize,
                                                  .height = m_shadowMapSize,
                                                  .layers = 1};

        if(m_device->GetDevice().createFramebuffer(&frameBufferInfo, nullptr, &m_framebuffers[i]) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

} // namespace engine
