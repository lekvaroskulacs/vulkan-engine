#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "device.h"

namespace engine
{
class SwapChain
{
public:
    VkSwapchainKHR Get()
    {
        return m_swapChain;
    }

    VkExtent2D GetExtent()
    {
        return m_swapChainExtent;
    }

    VkRenderPass GetRenderPass()
    {
        return m_renderPass;
    }

    std::vector<vk::Framebuffer> GetFrameBuffers()
    {
        return m_swapChainFramebuffers;
    }

    std::vector<vk::Image> GetImages()
    {
        return m_swapChainImages;
    }

    explicit SwapChain(std::shared_ptr<Device> device, std::shared_ptr<Window> window)
        : m_device{device}
        , m_window{window}
    {
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDepthResources();
        createFrameBuffers();
    }

    ~SwapChain()
    {
        cleanupSwapChain();
        vkDestroyRenderPass(m_device->GetDevice(), m_renderPass, nullptr);
    }

    void createImage(uint32_t width,
                     uint32_t height,
                     vk::Format format,
                     vk::ImageTiling tiling,
                     vk::ImageUsageFlags usage,
                     vk::MemoryPropertyFlags properties,
                     vk::Image& image,
                     vk::DeviceMemory& imageMemory)
    {
        vk::Extent3D extent{.width = width, .height = height, .depth = 1};

        vk::ImageCreateInfo imageInfo{
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = extent,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = tiling,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined,
        };

        if(m_device->GetDevice().createImage(&imageInfo, nullptr, &image) != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to create image!");
        }

        vk::MemoryRequirements memRequirements;
        m_device->GetDevice().getImageMemoryRequirements(image, &memRequirements);

        vk::MemoryAllocateInfo allocInfo{.allocationSize = memRequirements.size,
                                         .memoryTypeIndex = m_device->findMemoryType(
                                             memRequirements.memoryTypeBits, properties)};

        if(m_device->GetDevice().allocateMemory(&allocInfo, nullptr, &imageMemory) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to allocate image memory!");
        }

        m_device->GetDevice().bindImageMemory(image, imageMemory, 0);
    }

    vk::ImageView
    createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags)
    {
        vk::ImageSubresourceRange subresourceRange{.aspectMask = aspectFlags,
                                                   .baseMipLevel = 0,
                                                   .levelCount = 1,
                                                   .baseArrayLayer = 0,
                                                   .layerCount = 1};

        vk::ImageViewCreateInfo viewInfo{.image = image,
                                         .viewType = vk::ImageViewType::e2D,
                                         .format = format,
                                         .subresourceRange = subresourceRange};

        vk::ImageView imageView;
        if(m_device->GetDevice().createImageView(&viewInfo, nullptr, &imageView) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to create image view!");
        }

        return imageView;
    }

    void recreateSwapChain()
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
        createImageViews();
        createDepthResources();
        createFrameBuffers();
    }

private:
    vk::SurfaceFormatKHR
    chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        for(const auto& availableFormat : availableFormats)
        {
            if(availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
               availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    vk::PresentModeKHR
    chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
        for(const auto& availablePresentMode : availablePresentModes)
        {
            if(availablePresentMode == vk::PresentModeKHR::eMailbox)
            {
                return availablePresentMode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(m_window->Get(), &width, &height);

            vk::Extent2D actualExtent = {static_cast<uint32_t>(width),
                                         static_cast<uint32_t>(height)};

            actualExtent.width = std::clamp(actualExtent.width,
                                            capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height,
                                             capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);
            return actualExtent;
        }
    }

    void createSwapChain()
    {
        engine::utils::SwapChainSupportDetails swapChainSupport =
            engine::utils::SwapChainSupportDetails::querySwapChainSupport(
                m_device->GetPhysicalDevice(), m_device->GetSurface());

        vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.m_formats);
        vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.m_presentModes);
        vk::Extent2D extent = chooseSwapExtent(swapChainSupport.m_capabilities);

        uint32_t imageCount = swapChainSupport.m_capabilities.minImageCount + 1;
        if(swapChainSupport.m_capabilities.maxImageCount > 0 &&
           imageCount > swapChainSupport.m_capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.m_capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo{.surface = m_device->GetSurface(),
                                              .minImageCount = imageCount,
                                              .imageFormat = surfaceFormat.format,
                                              .imageColorSpace = surfaceFormat.colorSpace,
                                              .imageExtent = extent,
                                              .imageArrayLayers = 1,
                                              .imageUsage =
                                                  vk::ImageUsageFlagBits::eColorAttachment};

        engine::utils::QueueFamilyIndices indices =
            engine::utils::QueueFamilyIndices::findQueueFamilies(m_device->GetPhysicalDevice(),
                                                                 m_device->GetSurface());
        uint32_t queueFamilyIndices[] = {indices.m_graphicsFamily.value(),
                                         indices.m_presentFamily.value()};

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

        if(m_device->GetDevice().createSwapchainKHR(&createInfo, nullptr, &m_swapChain) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create swap chain!");
        }

        [[maybe_unused]] auto result =
            m_device->GetDevice().getSwapchainImagesKHR(m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        result = m_device->GetDevice().getSwapchainImagesKHR(
            m_swapChain, &imageCount, m_swapChainImages.data());
        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    void createImageViews()
    {
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for(size_t i = 0; i < m_swapChainImages.size(); i++)
        {
            m_swapChainImageViews[i] = createImageView(
                m_swapChainImages[i], m_swapChainImageFormat, vk::ImageAspectFlagBits::eColor);
        }
    }

    void createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{.format = m_swapChainImageFormat,
                                                  .samples = vk::SampleCountFlagBits::e1,
                                                  .loadOp = vk::AttachmentLoadOp::eClear,
                                                  .storeOp = vk::AttachmentStoreOp::eStore,
                                                  .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                                  .stencilStoreOp =
                                                      vk::AttachmentStoreOp::eDontCare,
                                                  .initialLayout = vk::ImageLayout::eUndefined,
                                                  .finalLayout = vk::ImageLayout::ePresentSrcKHR};

        vk::AttachmentReference colorAttachmentRef{
            .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

        vk::AttachmentDescription depthAttachment{
            .format = findDepthFormat(),
            .samples = vk::SampleCountFlagBits::e1,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

        vk::AttachmentReference depthAttachmentRef{
            .attachment = 1, .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

        vk::SubpassDescription subpass{
            .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
            .pDepthStencilAttachment = &depthAttachmentRef,
        };

        vk::SubpassDependency dependency{
            .srcSubpass = vk::SubpassExternal,
            .dstSubpass = 0,
            .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                            vk::PipelineStageFlagBits::eLateFragmentTests,
            .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                            vk::PipelineStageFlagBits::eEarlyFragmentTests,
            .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite |
                             vk::AccessFlagBits::eDepthStencilAttachmentWrite};

        std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        vk::RenderPassCreateInfo renderPassInfo{.attachmentCount =
                                                    static_cast<uint32_t>(attachments.size()),
                                                .pAttachments = attachments.data(),
                                                .subpassCount = 1,
                                                .pSubpasses = &subpass,
                                                .dependencyCount = 1,
                                                .pDependencies = &dependency};

        if(m_device->GetDevice().createRenderPass(&renderPassInfo, nullptr, &m_renderPass) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    void createFrameBuffers()
    {
        m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

        for(size_t i = 0; i < m_swapChainImageViews.size(); i++)
        {
            std::array<vk::ImageView, 2> attachments = {m_swapChainImageViews[i], m_depthImageView};

            vk::FramebufferCreateInfo frameBufferInfo{.renderPass = m_renderPass,
                                                      .attachmentCount =
                                                          static_cast<uint32_t>(attachments.size()),
                                                      .pAttachments = attachments.data(),
                                                      .width = m_swapChainExtent.width,
                                                      .height = m_swapChainExtent.height,
                                                      .layers = 1};

            if(m_device->GetDevice().createFramebuffer(
                   &frameBufferInfo, nullptr, &m_swapChainFramebuffers[i]) != vk::Result::eSuccess)
            {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    void createDepthResources()
    {
        vk::Format depthFormat = findDepthFormat();

        createImage(m_swapChainExtent.width,
                    m_swapChainExtent.height,
                    depthFormat,
                    vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment,
                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                    m_depthImage,
                    m_depthImageMemory);
        m_depthImageView =
            createImageView(m_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
    }

    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates,
                                   vk::ImageTiling tiling,
                                   vk::FormatFeatureFlags features)
    {
        for(vk::Format format : candidates)
        {
            vk::FormatProperties props;
            m_device->GetPhysicalDevice().getFormatProperties(format, &props);

            if(tiling == vk::ImageTiling::eLinear &&
               (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if(tiling == vk::ImageTiling::eOptimal &&
                    (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    vk::Format findDepthFormat()
    {
        return findSupportedFormat(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    void cleanupSwapChain()
    {
        m_device->GetDevice().destroyImageView(m_depthImageView, nullptr);
        m_device->GetDevice().destroyImage(m_depthImage, nullptr);
        m_device->GetDevice().freeMemory(m_depthImageMemory, nullptr);

        for(auto framebuffer : m_swapChainFramebuffers)
        {
            m_device->GetDevice().destroyFramebuffer(framebuffer, nullptr);
        }

        for(auto imageView : m_swapChainImageViews)
        {
            m_device->GetDevice().destroyImageView(imageView, nullptr);
        }

        m_device->GetDevice().destroySwapchainKHR(m_swapChain, nullptr);
    }

    std::shared_ptr<Device> m_device;
    std::shared_ptr<Window> m_window;

    vk::SwapchainKHR m_swapChain;
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::ImageView> m_swapChainImageViews;
    std::vector<vk::Framebuffer> m_swapChainFramebuffers;

    vk::Image m_depthImage;
    vk::DeviceMemory m_depthImageMemory;
    vk::ImageView m_depthImageView;

    vk::RenderPass m_renderPass;
};
} // namespace engine