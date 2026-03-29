#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#pragma GCC system_header
#include <tiny_obj_loader.h>

#include <chrono>

#include <algorithm>
#include <array>
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

#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "utils.h"
#include "window.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

const std::vector<const char*> g_validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> g_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool g_enableValidationLayers = false;
#else
const bool g_enableValidationLayers = true;
#endif

namespace engine
{
class Device
{
public:
    vk::Device GetDevice()
    {
        return m_device;
    }

    vk::PhysicalDevice GetPhysicalDevice()
    {
        return m_physicalDevice;
    }

    vk::SurfaceKHR GetSurface()
    {
        return m_surface;
    }

    vk::Queue GetPresentQueue()
    {
        return m_presentQueue;
    }

    vk::Queue GetGraphicsQueue()
    {
        return m_graphicsQueue;
    }

    explicit Device(std::shared_ptr<Window> window)
        : m_window{window}
    {
        VULKAN_HPP_DEFAULT_DISPATCHER.init();
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    ~Device()
    {
        m_device.destroy(nullptr);
        if(g_enableValidationLayers)
        {
            m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger, nullptr);
        }
        m_instance.destroySurfaceKHR(m_surface, nullptr);
        m_instance.destroy(nullptr);
        glfwTerminate();
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        vk::PhysicalDeviceMemoryProperties memProperties;
        m_physicalDevice.getMemoryProperties(&memProperties);

        for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if((typeFilter & (1 << i)) &&
               (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createBuffer(vk::DeviceSize size,
                      vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags properties,
                      vk::Buffer& buffer,
                      vk::DeviceMemory& bufferMemory)
    {
        vk::BufferCreateInfo bufferInfo{
            .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};

        if(m_device.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to create buffer!");
        }

        vk::MemoryRequirements memRequirements;
        m_device.getBufferMemoryRequirements(buffer, &memRequirements);

        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)};

        if(m_device.allocateMemory(&allocInfo, nullptr, &bufferMemory) != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        m_device.bindBufferMemory(buffer, bufferMemory, 0);
    }

private:
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL
    debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  vk::DebugUtilsMessageTypeFlagsEXT messageType,
                  const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void* pUserData)
    {
        std::string sSeverity{};
        switch(messageSeverity)
        {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: {
            sSeverity = "VERBOSE";
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: {
            sSeverity = "WARNING";
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: {
            sSeverity = "ERROR";
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: {
            sSeverity = "INFO";
            break;
        }
        }

        std::cerr << "Validation layer\t| " << sSeverity + "\t| " << pCallbackData->pMessage
                  << std::endl;

        return VK_FALSE;
    }

    void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                      .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                      .pfnUserCallback = debugCallback};
    }

    void setupDebugMessenger()
    {
        if(!g_enableValidationLayers)
            return;

        vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        if(m_instance.createDebugUtilsMessengerEXT(&createInfo, nullptr, &m_debugMessenger) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to set up debug messenger!");
        }
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        [[maybe_unused]] auto result = vk::enumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<vk::LayerProperties> availableLayers(layerCount);
        result = vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for(const char* layerName : g_validationLayers)
        {
            bool layerFound = false;

            for(const auto& layerProperties : availableLayers)
            {
                if(strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if(!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if(g_enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkDeviceExtensionSupport(vk::PhysicalDevice device)
    {
        uint32_t extensionCount;
        [[maybe_unused]] auto result =
            device.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<vk::ExtensionProperties> availableExtensionProperties(extensionCount);
        result = device.enumerateDeviceExtensionProperties(
            nullptr, &extensionCount, availableExtensionProperties.data());

        std::set<std::string> requiredExtensions{g_deviceExtensions.begin(),
                                                 g_deviceExtensions.end()};

        for(const auto& extension : availableExtensionProperties)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    void createInstance()
    {
        if(g_enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("Validation layers requested, but not available!");
        }

        uint32_t extensionCount = 0;
        std::vector<vk::ExtensionProperties> extensionList{extensionCount};
        [[maybe_unused]] auto result = vk::enumerateInstanceExtensionProperties(
            nullptr, &extensionCount, extensionList.data());

        for(const auto& extension : extensionList)
        {
            std::cout << '\t' << extension.extensionName << '\n';
        }

        vk::ApplicationInfo appInfo{.pApplicationName = "Engine",
                                    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                    .pEngineName = "No Engine",
                                    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                    .apiVersion = vk::ApiVersion10};

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        auto extensions = getRequiredExtensions();

        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if(g_enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
            createInfo.ppEnabledLayerNames = g_validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        m_instance = vk::createInstance(createInfo);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
    }

    void createSurface()
    {
        auto* surface = reinterpret_cast<VkSurfaceKHR*>(&m_surface);
        if(glfwCreateWindowSurface(m_instance, m_window->Get(), nullptr, surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    int rateDeviceSuitability(vk::PhysicalDevice device)
    {
        vk::PhysicalDeviceProperties deviceProperties;
        device.getProperties(&deviceProperties);

        vk::PhysicalDeviceFeatures deviceFeatures;
        device.getFeatures(&deviceFeatures);

        int score = 0;

        if(deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        {
            score += 1000;
        }

        score += deviceProperties.limits.maxImageDimension2D;

        utils::QueueFamilyIndices indices =
            utils::QueueFamilyIndices::findQueueFamilies(device, m_surface);
        if(!indices.isComplete())
        {
            return 0;
        }

        if(!checkDeviceExtensionSupport(device))
        {
            return 0;
        }

        utils::SwapChainSupportDetails swapChainSupport =
            utils::SwapChainSupportDetails::querySwapChainSupport(device, m_surface);
        if(swapChainSupport.m_formats.empty() || swapChainSupport.m_presentModes.empty())
        {
            return 0;
        }

        vk::PhysicalDeviceFeatures supportedFeatures;
        device.getFeatures(&supportedFeatures);
        if(!supportedFeatures.samplerAnisotropy)
        {
            return 0;
        }

        return score;
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        [[maybe_unused]] auto result = m_instance.enumeratePhysicalDevices(&deviceCount, nullptr);

        if(deviceCount == 0)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<vk::PhysicalDevice> devices(deviceCount);
        result = m_instance.enumeratePhysicalDevices(&deviceCount, devices.data());

        std::multimap<int, vk::PhysicalDevice> candidates;

        for(const auto& device : devices)
        {
            int score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }

        if(candidates.rbegin()->first > 0)
        {
            m_physicalDevice = candidates.rbegin()->second;
        }
        else
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }

        if(m_physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice()
    {
        utils::QueueFamilyIndices indices =
            utils::QueueFamilyIndices::findQueueFamilies(m_physicalDevice, m_surface);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.m_graphicsFamily.value(),
                                                  indices.m_presentFamily.value()};

        float queuePriority = 1.0f;
        for(const auto queueFamily : uniqueQueueFamilies)
        {
            vk::DeviceQueueCreateInfo queueCreateInfo{.queueFamilyIndex = queueFamily,
                                                      .queueCount = 1,
                                                      .pQueuePriorities = &queuePriority};
            queueCreateInfos.push_back(queueCreateInfo);
        }

        vk::PhysicalDeviceFeatures deviceFeatures{.samplerAnisotropy = VK_TRUE};

        vk::DeviceCreateInfo createInfo{
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledExtensionCount = static_cast<uint32_t>(g_deviceExtensions.size()),
            .ppEnabledExtensionNames = g_deviceExtensions.data(),
            .pEnabledFeatures = &deviceFeatures};

        if(g_enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
            createInfo.ppEnabledLayerNames = g_validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if(m_physicalDevice.createDevice(&createInfo, {}, &m_device) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create logical device!");
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);

        m_device.getQueue(indices.m_graphicsFamily.value(), 0, &m_graphicsQueue);
        m_device.getQueue(indices.m_presentFamily.value(), 0, &m_presentQueue);
    }

    std::shared_ptr<Window> m_window;

    vk::Instance m_instance;
    vk::DebugUtilsMessengerEXT m_debugMessenger;
    vk::PhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    vk::Device m_device;
    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;
    vk::SurfaceKHR m_surface;
}; // namespace engine

} // namespace engine