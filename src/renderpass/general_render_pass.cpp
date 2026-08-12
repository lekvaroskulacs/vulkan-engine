#include "general_render_pass.h"
#include <array>
#include <stdexcept>

namespace engine
{

vk::Extent2D GeneralRenderPass::GetExtent() const
{
    return m_swapChain->GetExtent();
}

vk::Framebuffer GeneralRenderPass::GetFrameBuffer(uint32_t instance, uint32_t imageIndex) const
{
    return m_framebuffers[imageIndex];
}

std::vector<vk::ClearValue> GeneralRenderPass::GetClearValues() const
{
    vk::ClearValue colorClear{.color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f})};
    vk::ClearValue depthClear{.depthStencil = vk::ClearDepthStencilValue{1.0f, 0}};
    return {colorClear, depthClear};
}

GeneralRenderPass::GeneralRenderPass(std::shared_ptr<Device> device, std::shared_ptr<SwapChain> swapChain)
    : RenderPass(device)
    , m_swapChain{swapChain}
{
    createRenderPass();
    createDepthResources();
    createFrameBuffers();
}

GeneralRenderPass::~GeneralRenderPass()
{
    cleanupFrameBufferResources();
}

void GeneralRenderPass::recreate()
{
    cleanupFrameBufferResources();
    createDepthResources();
    createFrameBuffers();
}

void GeneralRenderPass::createRenderPass()
{
    vk::AttachmentDescription colorAttachment{.format = m_swapChain->GetImageFormat(),
                                              .samples = vk::SampleCountFlagBits::e1,
                                              .loadOp = vk::AttachmentLoadOp::eClear,
                                              .storeOp = vk::AttachmentStoreOp::eStore,
                                              .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                              .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                                              .initialLayout = vk::ImageLayout::eUndefined,
                                              .finalLayout = vk::ImageLayout::ePresentSrcKHR};

    vk::AttachmentReference colorAttachmentRef{.attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

    vk::AttachmentDescription depthAttachment{.format = m_device->findDepthFormat(),
                                              .samples = vk::SampleCountFlagBits::e1,
                                              .loadOp = vk::AttachmentLoadOp::eClear,
                                              .storeOp = vk::AttachmentStoreOp::eDontCare,
                                              .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                              .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                                              .initialLayout = vk::ImageLayout::eUndefined,
                                              .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

    vk::AttachmentReference depthAttachmentRef{.attachment = 1, .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    vk::SubpassDependency dependency{
        .srcSubpass = vk::SubpassExternal,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite};

    std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    vk::RenderPassCreateInfo renderPassInfo{.attachmentCount = static_cast<uint32_t>(attachments.size()),
                                            .pAttachments = attachments.data(),
                                            .subpassCount = 1,
                                            .pSubpasses = &subpass,
                                            .dependencyCount = 1,
                                            .pDependencies = &dependency};

    if(m_device->GetDevice().createRenderPass(&renderPassInfo, nullptr, &m_renderPass) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void GeneralRenderPass::createDepthResources()
{
    vk::Format depthFormat = m_device->findDepthFormat();

    m_device->createImage(m_swapChain->GetExtent().width,
                          m_swapChain->GetExtent().height,
                          depthFormat,
                          vk::ImageTiling::eOptimal,
                          vk::ImageUsageFlagBits::eDepthStencilAttachment,
                          vk::MemoryPropertyFlagBits::eDeviceLocal,
                          m_depthImage,
                          m_depthImageAllocation);
    m_depthImageView = m_device->createImageView(m_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

void GeneralRenderPass::createFrameBuffers()
{
    auto imageViews = m_swapChain->GetImageViews();
    m_framebuffers.resize(imageViews.size());

    for(size_t i = 0; i < imageViews.size(); i++)
    {
        std::array<vk::ImageView, 2> attachments = {imageViews[i], m_depthImageView};

        vk::FramebufferCreateInfo frameBufferInfo{.renderPass = m_renderPass,
                                                  .attachmentCount = static_cast<uint32_t>(attachments.size()),
                                                  .pAttachments = attachments.data(),
                                                  .width = m_swapChain->GetExtent().width,
                                                  .height = m_swapChain->GetExtent().height,
                                                  .layers = 1};

        if(m_device->GetDevice().createFramebuffer(&frameBufferInfo, nullptr, &m_framebuffers[i]) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void GeneralRenderPass::cleanupFrameBufferResources()
{
    m_device->GetDevice().destroyImageView(m_depthImageView, nullptr);
    m_device->destroyImage(m_depthImage, m_depthImageAllocation);

    for(auto& framebuffer : m_framebuffers)
    {
        m_device->GetDevice().destroyFramebuffer(framebuffer, nullptr);
    }
    m_framebuffers.clear();
}

} // namespace engine
