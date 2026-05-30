#include "../include/swap_chain.h"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace engine
{

SwapChain::SwapChain(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, std::shared_ptr<Window> window)
    : m_device{device}
    , m_window{window}
    , m_commandBuffer{commandBuffer}
{
    createSwapChain();
    createGeneralImageViews();
    createGeneralRenderPass();
    createDepthResources();
    createGeneralFrameBuffers();
    // createShadowMapRenderTargets();
    // createShadowMapRenderPass();
    // createShadowMapFrameBuffers();
    createOmniShadowMapRenderTargets();
    createOmniShadowMapRenderPass();
    createOmniShadowMapFrameBuffers();
}

SwapChain::~SwapChain()
{
    cleanupSwapChain();
    for(auto& pass : m_renderPasses)
    {
        m_device->GetDevice().destroyRenderPass(pass.second, nullptr);
    }
}

VkSwapchainKHR SwapChain::Get()
{
    return m_swapChain;
}

vk::Extent2D SwapChain::GetExtent()
{
    return m_swapChainExtent;
}

vk::RenderPass SwapChain::GetRenderPass(RenderPassStage stage)
{
    return m_renderPasses.at(stage);
}

std::vector<vk::Framebuffer> SwapChain::GetFrameBuffers(RenderPassStage stage)
{
    return m_framebuffers[stage];
}

std::vector<vk::Image> SwapChain::GetImages(RenderPassStage stage)
{
    if(stage == RenderPassStage::General)
    {
        return m_presentImages;
    }

    return {};
}

Texture* SwapChain::GetRenderTarget(RenderPassStage stage)
{
    return m_renderTargets.at(stage).at(0).get();
}

void SwapChain::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window->Get(), &width, &height);
    while(width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_window->Get(), &width, &height);
        glfwWaitEvents();
    }
    m_device->GetDevice().waitIdle();
    cleanupSwapChain();
    createSwapChain();
    createGeneralImageViews();
    createDepthResources();
    createGeneralFrameBuffers();
    // createShadowMapRenderTargets();
    // createShadowMapRenderPass();
    // createShadowMapFrameBuffers();
    createOmniShadowMapRenderTargets();
    createOmniShadowMapRenderPass();
    createOmniShadowMapFrameBuffers();
}

vk::SurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    for(const auto& availableFormat : availableFormats)
    {
        if(availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

// TODO: make vsync optional
vk::PresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    // for(const auto& availablePresentMode : availablePresentModes)
    // {
    //     if(availablePresentMode == vk::PresentModeKHR::eMailbox)
    //     {
    //         return availablePresentMode;
    //     }
    // }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(m_window->Get(), &width, &height);

        vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

void SwapChain::createSwapChain()
{
    engine::utils::SwapChainSupportDetails swapChainSupport =
        engine::utils::SwapChainSupportDetails::querySwapChainSupport(m_device->GetPhysicalDevice(), m_device->GetSurface());

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.m_formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.m_presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.m_capabilities);

    m_imageCount = swapChainSupport.m_capabilities.minImageCount + 1;
    if(swapChainSupport.m_capabilities.maxImageCount > 0 && m_imageCount > swapChainSupport.m_capabilities.maxImageCount)
    {
        m_imageCount = swapChainSupport.m_capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{.surface = m_device->GetSurface(),
                                          .minImageCount = m_imageCount,
                                          .imageFormat = surfaceFormat.format,
                                          .imageColorSpace = surfaceFormat.colorSpace,
                                          .imageExtent = extent,
                                          .imageArrayLayers = 1,
                                          .imageUsage = vk::ImageUsageFlagBits::eColorAttachment};

    engine::utils::QueueFamilyIndices indices =
        engine::utils::QueueFamilyIndices::findQueueFamilies(m_device->GetPhysicalDevice(), m_device->GetSurface());
    uint32_t queueFamilyIndices[] = {indices.m_graphicsFamily.value(), indices.m_presentFamily.value()};

    if(indices.m_graphicsFamily != indices.m_presentFamily)
    {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.m_capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if(m_device->GetDevice().createSwapchainKHR(&createInfo, nullptr, &m_swapChain) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;

    createGeneralImages(m_imageCount);
}

void SwapChain::createGeneralImages(uint32_t imageCount)
{
    [[maybe_unused]] auto result = m_device->GetDevice().getSwapchainImagesKHR(m_swapChain, &imageCount, nullptr);
    m_presentImages.resize(imageCount);
    result = m_device->GetDevice().getSwapchainImagesKHR(m_swapChain, &imageCount, m_presentImages.data());
}

void SwapChain::createGeneralImageViews()
{
    m_presentImageViews.resize(m_presentImages.size());

    for(size_t i = 0; i < m_presentImages.size(); i++)
    {
        m_presentImageViews[i] =
            m_device->createImageView(m_presentImages[i], m_swapChainImageFormat, vk::ImageAspectFlagBits::eColor);
    }
}

void SwapChain::createGeneralRenderPass()
{
    vk::AttachmentDescription colorAttachment{.format = m_swapChainImageFormat,
                                              .samples = vk::SampleCountFlagBits::e1,
                                              .loadOp = vk::AttachmentLoadOp::eClear,
                                              .storeOp = vk::AttachmentStoreOp::eStore,
                                              .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                              .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                                              .initialLayout = vk::ImageLayout::eUndefined,
                                              .finalLayout = vk::ImageLayout::ePresentSrcKHR};

    vk::AttachmentReference colorAttachmentRef{.attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

    vk::AttachmentDescription depthAttachment{.format = findDepthFormat(),
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

    if(m_device->GetDevice().createRenderPass(&renderPassInfo, nullptr, &m_renderPasses[RenderPassStage::General]) !=
       vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void SwapChain::createGeneralFrameBuffers()
{
    m_framebuffers[RenderPassStage::General].resize(m_presentImageViews.size());

    for(size_t i = 0; i < m_presentImageViews.size(); i++)
    {
        std::array<vk::ImageView, 2> attachments = {m_presentImageViews[i], m_depthImageView};

        vk::FramebufferCreateInfo frameBufferInfo{.renderPass = m_renderPasses[RenderPassStage::General],
                                                  .attachmentCount = static_cast<uint32_t>(attachments.size()),
                                                  .pAttachments = attachments.data(),
                                                  .width = m_swapChainExtent.width,
                                                  .height = m_swapChainExtent.height,
                                                  .layers = 1};

        if(m_device->GetDevice().createFramebuffer(&frameBufferInfo, nullptr, &m_framebuffers[RenderPassStage::General][i]) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void SwapChain::createDepthResources()
{
    vk::Format depthFormat = findDepthFormat();

    m_device->createImage(m_swapChainExtent.width,
                          m_swapChainExtent.height,
                          depthFormat,
                          vk::ImageTiling::eOptimal,
                          vk::ImageUsageFlagBits::eDepthStencilAttachment,
                          vk::MemoryPropertyFlagBits::eDeviceLocal,
                          m_depthImage,
                          m_depthImageAllocation);
    m_depthImageView = m_device->createImageView(m_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

    m_shadowDepthStencil =
        std::make_unique<Texture>(m_device,
                                  m_commandBuffer,
                                  std::move(TextureDepthStencilParams{
                                      .m_width = m_shadowMapSize, .m_height = m_shadowMapSize, .m_depthFormat = findDepthFormat()}));
    m_shadowDepthStencil->transitionImageLayout(
        vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal, 1, vk::ImageAspectFlagBits::eDepth);
}

void SwapChain::createShadowMapRenderTargets()
{
    // maybe also pass format
    TextureFramebufferParams params{.m_width = m_shadowMapSize,
                                    .m_height = m_shadowMapSize,
                                    .m_usageFlags = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
                                    .m_aspectFlags = vk::ImageAspectFlagBits::eDepth,
                                    .m_format = findDepthFormat()};

    for(int i = 0; i < m_imageCount; ++i)
    {
        m_renderTargets[RenderPassStage::ShadowMap].push_back(
            std::move(std::make_unique<Texture>(m_device, m_commandBuffer, std::move(params))));
    }
}

void SwapChain::createShadowMapRenderPass()
{
    vk::AttachmentDescription depthAttachment{.format = findDepthFormat(),
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

    if(m_device->GetDevice().createRenderPass(&renderPassInfo, nullptr, &m_renderPasses[RenderPassStage::ShadowMap]) !=
       vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void SwapChain::createShadowMapFrameBuffers()
{
    m_framebuffers[RenderPassStage::ShadowMap].resize(m_imageCount);

    for(int i = 0; i < 1; ++i)
    {
        vk::FramebufferCreateInfo frameBufferInfo{.renderPass = m_renderPasses[RenderPassStage::ShadowMap],
                                                  .attachmentCount = 1,
                                                  .pAttachments =
                                                      m_renderTargets.at(RenderPassStage::ShadowMap).at(i)->GetImageView_ptr(),
                                                  .width = m_shadowMapSize,
                                                  .height = m_shadowMapSize,
                                                  .layers = 1}; //if cube then 6

        if(m_device->GetDevice().createFramebuffer(&frameBufferInfo, nullptr, &m_framebuffers[RenderPassStage::ShadowMap][i]) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void SwapChain::createOmniShadowMapRenderTargets()
{
    TextureFramebufferParams params{.m_width = m_shadowMapSize,
                                    .m_height = m_shadowMapSize,
                                    .m_usageFlags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                    .m_aspectFlags = vk::ImageAspectFlagBits::eColor,
                                    .m_format = vk::Format::eR32Sfloat,
                                    .m_isCube = true};

    for(int i = 0; i < 1; ++i)
    {
        auto tex = std::make_unique<Texture>(m_device, m_commandBuffer, std::move(params));
        tex->transitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, 6);
        m_renderTargets[RenderPassStage::ShadowMap].push_back(std::move(tex));
    }

    //also have 6 image views for all cube faces
    for(uint32_t i = 0; i < 6; ++i)
    {
        TextureViewOnlyParams face{.m_reference = m_renderTargets[RenderPassStage::ShadowMap].at(0).get(), .m_baseArrayLayer = i};

        auto tex = std::make_unique<Texture>(m_device, m_commandBuffer, std::move(face));
        m_renderTargets[RenderPassStage::ShadowMap].push_back(std::move(tex));
    }
}

void SwapChain::createOmniShadowMapRenderPass()
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

    vk::AttachmentDescription depthAttachment{.format = findDepthFormat(),
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

    if(m_device->GetDevice().createRenderPass(&renderPassInfo, nullptr, &m_renderPasses[RenderPassStage::ShadowMap]) !=
       vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void SwapChain::createOmniShadowMapFrameBuffers()
{
    m_framebuffers[RenderPassStage::ShadowMap].resize(6);

    vk::ImageView attachments[2];
    attachments[1] = m_shadowDepthStencil->GetImageView();
    for(int i = 0; i < 6; ++i)
    {
        attachments[0] = m_renderTargets.at(RenderPassStage::ShadowMap).at(i + 1)->GetImageView();
        vk::FramebufferCreateInfo frameBufferInfo{.renderPass = m_renderPasses[RenderPassStage::ShadowMap],
                                                  .attachmentCount = 2,
                                                  .pAttachments = attachments,
                                                  .width = m_shadowMapSize,
                                                  .height = m_shadowMapSize,
                                                  .layers = 1};

        if(m_device->GetDevice().createFramebuffer(&frameBufferInfo, nullptr, &m_framebuffers[RenderPassStage::ShadowMap][i]) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

vk::Format
SwapChain::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for(vk::Format format : candidates)
    {
        vk::FormatProperties props;
        m_device->GetPhysicalDevice().getFormatProperties(format, &props);

        if(tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if(tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

vk::Format SwapChain::findDepthFormat()
{
    return findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                               vk::ImageTiling::eOptimal,
                               vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void SwapChain::cleanupSwapChain()
{
    m_device->GetDevice().destroyImageView(m_depthImageView, nullptr);
    m_device->destroyImage(m_depthImage, m_depthImageAllocation);

    for(auto& [_, framebuffers] : m_framebuffers)
    {
        for(auto& framebuffer : framebuffers)
        {
            m_device->GetDevice().destroyFramebuffer(framebuffer, nullptr);
        }
    }

    for(auto& view : m_presentImageViews)
    {
        m_device->GetDevice().destroyImageView(view, nullptr);
    }

    m_device->GetDevice().destroySwapchainKHR(m_swapChain, nullptr);
}

} // namespace engine
