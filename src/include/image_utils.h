#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

#include "device.h"
#include <vulkan/vulkan.hpp>

namespace engine::utils
{

inline vk::ImageView createImageView(const vk::Device& device,
                                     vk::Image image,
                                     vk::Format format,
                                     vk::ImageAspectFlags aspectFlags,
                                     uint32_t layerCount = 1,
                                     vk::ImageViewType viewType = vk::ImageViewType::e2D)
{
    vk::ImageSubresourceRange subresourceRange{
        .aspectMask = aspectFlags, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = layerCount};
    vk::ImageViewCreateInfo viewInfo{.image = image, .viewType = viewType, .format = format, .subresourceRange = subresourceRange};
    vk::ImageView imageView;
    if(device.createImageView(&viewInfo, nullptr, &imageView) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create image view!");
    }
    return imageView;
}

inline void createImage(engine::Device& device,
                        uint32_t width,
                        uint32_t height,
                        vk::Format format,
                        vk::ImageTiling tiling,
                        vk::ImageUsageFlags usage,
                        vk::MemoryPropertyFlags properties,
                        vk::Image& image,
                        vk::DeviceMemory& imageMemory,
                        vk::ImageCreateFlags flags = {},
                        uint32_t arrayLayers = 1)
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
    if(device.GetDevice().createImage(&imageInfo, nullptr, &image) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create image!");
    }
    vk::MemoryRequirements memRequirements;
    device.GetDevice().getImageMemoryRequirements(image, &memRequirements);
    vk::MemoryAllocateInfo allocInfo{.allocationSize = memRequirements.size,
                                     .memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties)};
    if(device.GetDevice().allocateMemory(&allocInfo, nullptr, &imageMemory) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }
    device.GetDevice().bindImageMemory(image, imageMemory, 0);
}

} // namespace engine::utils