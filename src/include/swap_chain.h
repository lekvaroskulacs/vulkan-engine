#pragma once

#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "device.h"

namespace engine
{

enum class RenderPassStage
{
    GeneralObjects,
    ShadowMap
};

class SwapChain
{
public:
    VkSwapchainKHR Get();
    vk::Extent2D GetExtent();
    VkRenderPass GetRenderPass();
    std::vector<vk::Framebuffer> GetFrameBuffers();
    std::vector<vk::Image> GetImages();

    explicit SwapChain(std::shared_ptr<Device> device, std::shared_ptr<Window> window);
    ~SwapChain();

    void recreateSwapChain();

private:
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createFrameBuffers();
    void createDepthResources();
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::Format findDepthFormat();
    void cleanupSwapChain();

    std::shared_ptr<Device> m_device;
    std::shared_ptr<Window> m_window;

    vk::SwapchainKHR m_swapChain;
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::ImageView> m_swapChainImageViews;
    std::vector<vk::Framebuffer> m_swapChainFramebuffers;

    vk::Image m_depthImage;
    VmaAllocation m_depthImageAllocation;
    vk::ImageView m_depthImageView;

    vk::RenderPass m_renderPass;
    std::unordered_map<RenderPassStage, vk::RenderPass> m_renderPasses;
};
} // namespace engine