#pragma once

#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "../texture/texture.h"
#include "command_buffer.h"
#include "device.h"

namespace engine
{

enum class RenderPassStage
{
    General,
    ShadowMap
};

class SwapChain
{
public:
    VkSwapchainKHR Get();
    vk::Extent2D GetExtent();
    vk::RenderPass GetRenderPass(RenderPassStage stage);
    std::vector<vk::Framebuffer> GetFrameBuffers(RenderPassStage stage);
    std::vector<vk::Image> GetImages(RenderPassStage stage);
    Texture* GetRenderTarget(RenderPassStage stage);

    explicit SwapChain(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, std::shared_ptr<Window> window);
    ~SwapChain();

    void recreateSwapChain();

    constexpr static uint32_t m_shadowMapSize = 2024;

private:
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();

    // Create renderpass resources
    void createGeneralImages(uint32_t imageCount);
    void createGeneralImageViews();
    void createGeneralRenderPass();
    void createGeneralFrameBuffers();
    void createDepthResources();

    void createShadowMapRenderTargets();
    void createShadowMapRenderPass();
    void createShadowMapFrameBuffers();

    void createOmniShadowMapRenderTargets();
    void createOmniShadowMapRenderPass();
    void createOmniShadowMapFrameBuffers();

    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::Format findDepthFormat();
    void cleanupSwapChain();

    std::shared_ptr<Device> m_device;
    std::shared_ptr<Window> m_window;
    std::shared_ptr<CommandBuffer> m_commandBuffer;

    vk::SwapchainKHR m_swapChain;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;
    uint32_t m_imageCount;

    // For swapChainPresent
    std::vector<vk::Image> m_presentImages;
    std::vector<VmaAllocation> m_presentImageAllocations;
    std::vector<vk::ImageView> m_presentImageViews;

    // Generic renderpass resources
    std::unordered_map<RenderPassStage, std::vector<std::unique_ptr<Texture>>> m_renderTargets;
    std::unordered_map<RenderPassStage, std::vector<vk::Framebuffer>> m_framebuffers;

    vk::Image m_depthImage;
    VmaAllocation m_depthImageAllocation;
    vk::ImageView m_depthImageView;

    std::unique_ptr<Texture> m_shadowDepthStencil;

    std::unordered_map<RenderPassStage, vk::RenderPass> m_renderPasses;
};
} // namespace engine