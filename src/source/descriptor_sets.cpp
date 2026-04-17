#include "../include/descriptor_sets.h"
#include "../include/device.h"
//#include "../include/pipeline.h"
#include <array>
#include <stdexcept>

namespace engine
{

DescriptorSets::DescriptorSets(std::shared_ptr<Device> device,
                               std::shared_ptr<Pipeline> pipeline,
                               const std::vector<std::shared_ptr<Uniform>>& uniforms,
                               const std::vector<std::shared_ptr<Texture>>& textures)
    : m_device{device}
    , m_pipeline{pipeline}
{
    createDescriptorPool(uniforms, textures);
    createDescriptorSets(uniforms, textures);
}

DescriptorSets::~DescriptorSets()
{
    m_device->GetDevice().destroyDescriptorPool(m_descriptorPool, nullptr);
}

void DescriptorSets::createDescriptor()
{
    //TODO: create pipeline->descriptorsetlayout, create pool with correct types, create descriptorSets
}

void DescriptorSets::createDescriptorPool(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                                          const std::vector<std::shared_ptr<Texture>>& textures)
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * uniforms.size());
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * textures.size());

    vk::DescriptorPoolCreateInfo poolInfo{
        .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    if(m_device->GetDevice().createDescriptorPool(&poolInfo, nullptr, &m_descriptorPool) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void DescriptorSets::createDescriptorSets(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                                          const std::vector<std::shared_ptr<Texture>>& textures)
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_pipeline->GetDescriptorSetLayout());
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .pSetLayouts = layouts.data(),
    };

    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if(m_device->GetDevice().allocateDescriptorSets(&allocInfo, m_descriptorSets.data()) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<vk::WriteDescriptorSet> descriptorWrites{};
        descriptorWrites.resize(uniforms.size() + textures.size());
        int writesPos = 0;
        for(size_t j = 0; j < uniforms.size(); j++)
        {
            vk::DescriptorBufferInfo bufferInfo{
                .buffer = uniforms[j]->m_uniformBuffers[i],
                .offset = 0,
                .range = sizeof(engine::UniformBufferObject),
            };

            descriptorWrites[writesPos].sType = vk::StructureType::eWriteDescriptorSet;
            descriptorWrites[writesPos].dstSet = m_descriptorSets[i];
            descriptorWrites[writesPos].dstBinding = writesPos;
            descriptorWrites[writesPos].dstArrayElement = 0;
            descriptorWrites[writesPos].descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrites[writesPos].descriptorCount = 1;
            descriptorWrites[writesPos].pBufferInfo = &bufferInfo;

            writesPos++;
        }

        for(size_t j = 0; j < textures.size(); j++)
        {
            vk::DescriptorImageInfo imageInfo{
                .sampler = textures[j]->GetSampler(),
                .imageView = textures[j]->GetImageView(),
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            };

            descriptorWrites[writesPos].sType = vk::StructureType::eWriteDescriptorSet;
            descriptorWrites[writesPos].dstSet = m_descriptorSets[i];
            descriptorWrites[writesPos].dstBinding = writesPos;
            descriptorWrites[writesPos].dstArrayElement = 0;
            descriptorWrites[writesPos].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptorWrites[writesPos].descriptorCount = 1;
            descriptorWrites[writesPos].pImageInfo = &imageInfo;

            writesPos++;
        }

        m_device->GetDevice().updateDescriptorSets(
            static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

} // namespace engine
