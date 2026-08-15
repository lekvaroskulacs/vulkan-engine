#include <engine/swap_chain/swap_chain.h>
#include <algorithm>
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
    createImageViews();
}

SwapChain::~SwapChain()
{
    cleanupSwapChain();
}

VkSwapchainKHR SwapChain::Get()
{
    return m_swapChain;
}

vk::Extent2D SwapChain::GetExtent()
{
    return m_swapChainExtent;
}

vk::Format SwapChain::GetImageFormat()
{
    return m_swapChainImageFormat;
}

std::vector<vk::ImageView> SwapChain::GetImageViews()
{
    return m_presentImageViews;
}

std::vector<vk::Image> SwapChain::GetImages()
{
    return m_presentImages;
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
    createImageViews();
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

    createImages(m_imageCount);
}

void SwapChain::createImages(uint32_t imageCount)
{
    [[maybe_unused]] auto result = m_device->GetDevice().getSwapchainImagesKHR(m_swapChain, &imageCount, nullptr);
    m_presentImages.resize(imageCount);
    result = m_device->GetDevice().getSwapchainImagesKHR(m_swapChain, &imageCount, m_presentImages.data());
}

void SwapChain::createImageViews()
{
    m_presentImageViews.resize(m_presentImages.size());

    for(size_t i = 0; i < m_presentImages.size(); i++)
    {
        m_presentImageViews[i] =
            m_device->createImageView(m_presentImages[i], m_swapChainImageFormat, vk::ImageAspectFlagBits::eColor);
    }
}

void SwapChain::cleanupSwapChain()
{
    for(auto& view : m_presentImageViews)
    {
        m_device->GetDevice().destroyImageView(view, nullptr);
    }

    m_device->GetDevice().destroySwapchainKHR(m_swapChain, nullptr);
}

} // namespace engine
