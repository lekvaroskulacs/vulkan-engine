#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "command_buffer.h"
#include "device.h"

namespace engine
{

class SwapChain
{
public:
    VkSwapchainKHR Get();
    vk::Extent2D GetExtent();
    vk::Format GetImageFormat();
    std::vector<vk::ImageView> GetImageViews();
    std::vector<vk::Image> GetImages();

    explicit SwapChain(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, std::shared_ptr<Window> window);
    ~SwapChain();

    void recreateSwapChain();

private:
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();

    void createImages(uint32_t imageCount);
    void createImageViews();
    void cleanupSwapChain();

    std::shared_ptr<Device> m_device;
    std::shared_ptr<Window> m_window;
    std::shared_ptr<CommandBuffer> m_commandBuffer;

    vk::SwapchainKHR m_swapChain;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;
    uint32_t m_imageCount;

    std::vector<vk::Image> m_presentImages;
    std::vector<VmaAllocation> m_presentImageAllocations;
    std::vector<vk::ImageView> m_presentImageViews;
};
} // namespace engine
