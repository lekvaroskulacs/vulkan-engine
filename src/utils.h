#pragma once

#include <fstream>
#include <iostream>
#include <vector>

namespace engine::utils
{

std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if(!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> m_graphicsFamily;
    std::optional<uint32_t> m_presentFamily;

    bool isComplete()
    {
        return m_graphicsFamily.has_value() && m_presentFamily.has_value();
    }

    static QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        device.getQueueFamilyProperties(&queueFamilyCount, nullptr);

        std::vector<vk::QueueFamilyProperties> queueFamilies(queueFamilyCount);
        device.getQueueFamilyProperties(&queueFamilyCount, queueFamilies.data());

        int i = 0;
        for(const auto& queueFamily : queueFamilies)
        {
            if(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                indices.m_graphicsFamily = i;
            }

            vk::Bool32 presentSupport = false;
            device.getSurfaceSupportKHR(i, surface, &presentSupport);

            if(presentSupport)
            {
                indices.m_presentFamily = i;
            }

            if(indices.isComplete())
            {
                break;
            }

            ++i;
        }

        return indices;
    }
};

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR m_capabilities;
    std::vector<vk::SurfaceFormatKHR> m_formats;
    std::vector<vk::PresentModeKHR> m_presentModes;

    static SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device,
                                                         vk::SurfaceKHR surface)
    {
        SwapChainSupportDetails details;

        device.getSurfaceCapabilitiesKHR(surface, &details.m_capabilities);

        uint32_t formatCount;
        device.getSurfaceFormatsKHR(surface, &formatCount, nullptr);
        if(formatCount != 0)
        {
            details.m_formats.resize(formatCount);
            device.getSurfaceFormatsKHR(surface, &formatCount, details.m_formats.data());
        }

        uint32_t presentModeCount;
        device.getSurfacePresentModesKHR(surface, &presentModeCount, nullptr);
        if(presentModeCount != 0)
        {
            details.m_presentModes.resize(presentModeCount);
            device.getSurfacePresentModesKHR(surface, &formatCount, details.m_presentModes.data());
        }

        return details;
    }
};

} // namespace engine::utils