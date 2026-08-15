#include <engine/device/device.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace engine
{

const std::vector<const char*> g_validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> g_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool g_enableValidationLayers = false;
#else
const bool g_enableValidationLayers = true;
#endif

vk::Device Device::GetDevice()
{
    return m_device;
}
vk::PhysicalDevice Device::GetPhysicalDevice()
{
    return m_physicalDevice;
}
vk::SurfaceKHR Device::GetSurface()
{
    return m_surface;
}
vk::Queue Device::GetPresentQueue()
{
    return m_presentQueue;
}
vk::Queue Device::GetGraphicsQueue()
{
    return m_graphicsQueue;
}

Device::Device(std::shared_ptr<Window> window)
    : m_window{window}
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createMemoryAllocator();
}

Device::~Device()
{
    vmaDestroyAllocator(m_allocator);
    m_device.destroy(nullptr);
    if(g_enableValidationLayers)
    {
        m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger, nullptr);
    }
    m_instance.destroySurfaceKHR(m_surface, nullptr);
    m_instance.destroy(nullptr);
    glfwTerminate();
}

uint32_t Device::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties;
    m_physicalDevice.getMemoryProperties(&memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Device::createBuffer(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, VmaAllocation& allocation)
{
    vk::BufferCreateInfo bufferInfo{.size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(properties);

    if((properties & vk::MemoryPropertyFlagBits::eHostCoherent) != vk::MemoryPropertyFlags{})
    {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    // Little bit hacky because of vulkan.hpp types
    VkBufferCreateInfo info = bufferInfo;
    VkBuffer temp_buffer;
    vmaCreateBuffer(m_allocator, &info, &allocInfo, &temp_buffer, &allocation, nullptr);
    buffer = std::move(temp_buffer);
}

void Device::copyMemoryToAllocation(void* data, VmaAllocation& allocation, vk::DeviceSize size, vk::DeviceSize offset)
{
    vmaCopyMemoryToAllocation(m_allocator, data, allocation, offset, size);
}

void Device::destroyBuffer(vk::Buffer& buffer, VmaAllocation& allocation)
{
    vmaDestroyBuffer(m_allocator, buffer, allocation);
}

vk::ImageView Device::createImageView(
    vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t layerCount, vk::ImageViewType viewType)
{
    vk::ImageSubresourceRange subresourceRange{
        .aspectMask = aspectFlags, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = layerCount};
    vk::ImageViewCreateInfo viewInfo{.image = image, .viewType = viewType, .format = format, .subresourceRange = subresourceRange};
    vk::ImageView imageView;
    if(m_device.createImageView(&viewInfo, nullptr, &imageView) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create image view!");
    }
    return imageView;
}

void Device::createImage(uint32_t width,
                         uint32_t height,
                         vk::Format format,
                         vk::ImageTiling tiling,
                         vk::ImageUsageFlags usage,
                         vk::MemoryPropertyFlags properties,
                         vk::Image& image,
                         VmaAllocation& allocation,
                         vk::ImageCreateFlags flags,
                         uint32_t arrayLayers)
{
    vk::Extent3D extent{.width = width, .height = height, .depth = 1};
    vk::ImageCreateInfo imageInfo{
        .flags = flags,
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = arrayLayers,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(properties);

    VkImageCreateInfo vkImageInfo = imageInfo;
    VkImage temp_image;
    vmaCreateImage(m_allocator, &vkImageInfo, &allocInfo, &temp_image, &allocation, nullptr);
    image = std::move(temp_image);
}

void Device::destroyImage(vk::Image& image, VmaAllocation& allocation)
{
    vmaDestroyImage(m_allocator, image, allocation);
}

vk::Format Device::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for(vk::Format format : candidates)
    {
        vk::FormatProperties props;
        m_physicalDevice.getFormatProperties(format, &props);

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

vk::Format Device::findDepthFormat()
{
    return findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                               vk::ImageTiling::eOptimal,
                               vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Device::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                       vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                                       const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                       void* pUserData)
{
    std::string sSeverity{};
    switch(messageSeverity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        sSeverity = "VERBOSE";
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        sSeverity = "WARNING";
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        sSeverity = "ERROR";
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        sSeverity = "INFO";
        break;
    }

    std::cerr << "Validation layer\t| " << sSeverity + "\t| " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void Device::populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                     vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                     vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                  .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                 vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                  .pfnUserCallback = debugCallback};
}

void Device::setupDebugMessenger()
{
    if(!g_enableValidationLayers)
        return;

    vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if(m_instance.createDebugUtilsMessengerEXT(&createInfo, nullptr, &m_debugMessenger) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to set up debug messenger!");
    }
}

bool Device::checkValidationLayerSupport()
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

std::vector<const char*> Device::getRequiredExtensions()
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

bool Device::checkDeviceExtensionSupport(vk::PhysicalDevice device)
{
    uint32_t extensionCount;
    [[maybe_unused]] auto result = device.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<vk::ExtensionProperties> availableExtensionProperties(extensionCount);
    result = device.enumerateDeviceExtensionProperties(nullptr, &extensionCount, availableExtensionProperties.data());

    std::set<std::string> requiredExtensions{g_deviceExtensions.begin(), g_deviceExtensions.end()};

    for(const auto& extension : availableExtensionProperties)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void Device::createInstance()
{
    if(g_enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    uint32_t extensionCount = 0;
    std::vector<vk::ExtensionProperties> extensionList{extensionCount};
    [[maybe_unused]] auto result = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionList.data());

    for(const auto& extension : extensionList)
    {
        std::cout << '\t' << extension.extensionName << '\n';
    }

    vk::ApplicationInfo appInfo{.pApplicationName = "Engine",
                                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                .pEngineName = "No Engine",
                                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                .apiVersion = vk::ApiVersion12};

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

void Device::createSurface()
{
    auto* surface = reinterpret_cast<VkSurfaceKHR*>(&m_surface);
    if(glfwCreateWindowSurface(m_instance, m_window->Get(), nullptr, surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

int Device::rateDeviceSuitability(vk::PhysicalDevice device)
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

    utils::QueueFamilyIndices indices = utils::QueueFamilyIndices::findQueueFamilies(device, m_surface);
    if(!indices.isComplete())
    {
        return 0;
    }

    if(!checkDeviceExtensionSupport(device))
    {
        return 0;
    }

    utils::SwapChainSupportDetails swapChainSupport = utils::SwapChainSupportDetails::querySwapChainSupport(device, m_surface);
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

void Device::pickPhysicalDevice()
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

void Device::createLogicalDevice()
{
    utils::QueueFamilyIndices indices = utils::QueueFamilyIndices::findQueueFamilies(m_physicalDevice, m_surface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.m_graphicsFamily.value(), indices.m_presentFamily.value()};

    float queuePriority = 1.0f;
    for(const auto queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo{
            .queueFamilyIndex = queueFamily, .queueCount = 1, .pQueuePriorities = &queuePriority};
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{.samplerAnisotropy = VK_TRUE};

    vk::DeviceCreateInfo createInfo{.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
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

void Device::createMemoryAllocator()
{
    VmaAllocatorCreateInfo allocCreateInfo{};
    allocCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocCreateInfo.physicalDevice = m_physicalDevice;
    allocCreateInfo.device = m_device;
    allocCreateInfo.instance = m_instance;

    vmaCreateAllocator(&allocCreateInfo, &m_allocator);
}

void Device::initImGui(VkDescriptorPool descriptorPool, vk::RenderPass renderpass)
{
    ImGui_ImplGlfw_InitForVulkan(m_window->Get(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_2;
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_physicalDevice;
    init_info.Device = m_device;
    auto queueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(m_physicalDevice);
    init_info.QueueFamily = queueFamily;
    vkGetDeviceQueue(m_device, queueFamily, 0, &init_info.Queue);
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
    init_info.Allocator = nullptr;
    init_info.PipelineInfoMain.RenderPass = renderpass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = [](VkResult err) {
        if(err == VK_SUCCESS)
            return;
        fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
        if(err < 0)
            abort();
    };
    ImGui_ImplVulkan_Init(&init_info);
}

} // namespace engine