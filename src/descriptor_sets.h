#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "texture.h"
#include "unifoms.h"

namespace engine
{

class DescriptorSets
{
public:
    explicit DescriptorSets(std::shared_ptr<Device> device,
                            std::shared_ptr<Pipeline> pipeline,
                            const Uniforms& uniforms,
                            const Texture& textures)
        : m_device{device}
        , m_pipeline{pipeline}
    {
        createDescriptorPool();
        createDescriptorSets(uniforms, textures);
    }

    ~DescriptorSets()
    {
        m_device->GetDevice().destroyDescriptorPool(m_descriptorPool, nullptr);
    }

    void createDescriptor()
    {
        //TODO: create pipeline->descriptorsetlayout, create pool with correct types, create descriptorSets
    }

    void createDescriptorPool()
    {
        std::array<vk::DescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        vk::DescriptorPoolCreateInfo poolInfo{
            .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data(),
        };

        if(m_device->GetDevice().createDescriptorPool(&poolInfo, nullptr, &m_descriptorPool) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    void createDescriptorSets(const Uniforms& uniforms, const Texture& textures)
    {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                     m_pipeline->GetDescriptorSetLayout());
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .pSetLayouts = layouts.data(),
        };

        m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if(m_device->GetDevice().allocateDescriptorSets(&allocInfo, m_descriptorSets.data()) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vk::DescriptorBufferInfo bufferInfo{
                .buffer = uniforms.m_uniformBuffers[i],
                .offset = 0,
                .range = sizeof(engine::UniformBufferObject),
            };

            vk::DescriptorImageInfo imageInfo{
                .sampler = textures.GetSampler(),
                .imageView = textures.GetImageView(),
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            };

            std::array<vk::WriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = vk::StructureType::eWriteDescriptorSet;
            descriptorWrites[0].dstSet = m_descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = vk::StructureType::eWriteDescriptorSet;
            descriptorWrites[1].dstSet = m_descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            m_device->GetDevice().updateDescriptorSets(
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0,
                nullptr);
        }
    }

    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

private:
    std::shared_ptr<Device> m_device;
    std::shared_ptr<Pipeline> m_pipeline;
};

} // namespace engine
