#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "utils.h"
#include "vk_mem_alloc.h"
#include "window.h"

namespace engine
{

const int MAX_FRAMES_IN_FLIGHT = 2;
const std::string MODEL_PATH = "models/viking_room.obj";

extern const std::vector<const char*> g_validationLayers;
extern const std::vector<const char*> g_deviceExtensions;
extern const bool g_enableValidationLayers;

class Device
{
public:
    vk::Device GetDevice();
    vk::PhysicalDevice GetPhysicalDevice();
    vk::SurfaceKHR GetSurface();
    vk::Queue GetPresentQueue();
    vk::Queue GetGraphicsQueue();

    explicit Device(std::shared_ptr<Window> window);
    ~Device();

    void initImGui(VkDescriptorPool descriptorPool, vk::RenderPass renderpass);

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void createBuffer(vk::DeviceSize size,
                      vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags properties,
                      vk::Buffer& buffer,
                      VmaAllocation& allocation);

    void copyMemoryToAllocation(void* data, VmaAllocation& allocation, vk::DeviceSize size, vk::DeviceSize offset = 0);
    void destroyBuffer(vk::Buffer& buffer, VmaAllocation& allocation);

    vk::ImageView createImageView(vk::Image image,
                                  vk::Format format,
                                  vk::ImageAspectFlags aspectFlags,
                                  uint32_t layerCount = 1,
                                  vk::ImageViewType viewType = vk::ImageViewType::e2D);
    void createImage(uint32_t width,
                     uint32_t height,
                     vk::Format format,
                     vk::ImageTiling tiling,
                     vk::ImageUsageFlags usage,
                     vk::MemoryPropertyFlags properties,
                     vk::Image& image,
                     VmaAllocation& allocation,
                     vk::ImageCreateFlags flags = {},
                     uint32_t arrayLayers = 1);
    void destroyImage(vk::Image& image, VmaAllocation& allocation);

    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::Format findDepthFormat();

private:
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                          void* pUserData);

    void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
    void createInstance();
    void createSurface();
    int rateDeviceSuitability(vk::PhysicalDevice device);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createMemoryAllocator();

    std::shared_ptr<Window> m_window;
    vk::Instance m_instance;
    vk::DebugUtilsMessengerEXT m_debugMessenger;
    vk::PhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    vk::Device m_device;
    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;
    vk::SurfaceKHR m_surface;

public:
    VmaAllocator m_allocator;
};

} // namespace engine