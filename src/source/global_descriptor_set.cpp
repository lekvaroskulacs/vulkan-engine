#include <engine/global_descriptor_set/global_descriptor_set.h>

#include <algorithm>
#include <array>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine
{

GlobalDescriptorSet::GlobalDescriptorSet(std::shared_ptr<Device> device)
    : m_device{device}
{
    m_camera = std::make_unique<UniformCamera>(m_device);
    m_lights = std::make_unique<LightBuffer>(m_device);
    m_clusterBounds = std::make_unique<ClusterBoundsBuffer>(m_device);
    m_lightGrid = std::make_unique<LightGridBuffer>(m_device);
    m_lightIndices = std::make_unique<LightIndexBuffer>(m_device);

    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSets();
}

GlobalDescriptorSet::~GlobalDescriptorSet()
{
    m_device->GetDevice().destroyDescriptorPool(m_descriptorPool, nullptr);
    m_device->GetDevice().destroyDescriptorSetLayout(m_descriptorSetLayout, nullptr);
}

vk::DescriptorSetLayout GlobalDescriptorSet::GetLayout() const
{
    return m_descriptorSetLayout;
}

vk::DescriptorSet GlobalDescriptorSet::GetDescriptorSet(uint32_t frameIndex) const
{
    return m_descriptorSets[frameIndex];
}

// TODO: can these be more generic so i dont have to call them manually in main?
void GlobalDescriptorSet::updateCamera(uint32_t frameIndex, const Camera& camera)
{
    UniformCamera::UniformBufferObject ubo{};
    glm::mat4 rayDir{1.0f};
    rayDir = glm::translate(rayDir, camera.m_cameraPos);
    rayDir = glm::inverse(camera.m_proj * camera.m_view * rayDir);
    ubo.rayDir = rayDir;
    ubo.position = glm::vec4(camera.m_cameraPos, 1.0f);
    ubo.view = camera.m_view;
    ubo.proj = camera.m_proj;
    ubo.nearFar = glm::vec4(Camera::zNear, Camera::zFar, 0.0f, 0.0f);
    m_camera->updateBuffer(&ubo, frameIndex);
}

void GlobalDescriptorSet::updateLights(uint32_t frameIndex, const std::vector<Light>& lights)
{
    LightBuffer::LightBufferObject ssbo{};
    uint32_t count = std::min<uint32_t>(static_cast<uint32_t>(lights.size()), LightBuffer::MAX_LIGHTS);
    ssbo.count = count;
    std::copy(lights.begin(), lights.begin() + count, ssbo.lights);
    m_lights->updateBuffer(&ssbo, frameIndex);
}

// TODO: maybe have a more flexible way of adding global descriptors
// right now you need to add code in 3 different functions
void GlobalDescriptorSet::createDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 5> bindings{
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute,
            .pImmutableSamplers = nullptr,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute,
            .pImmutableSamplers = nullptr,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
            .pImmutableSamplers = nullptr,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 3,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment,
            .pImmutableSamplers = nullptr,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 4,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment,
            .pImmutableSamplers = nullptr,
        },
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    if(m_device->GetDevice().createDescriptorSetLayout(&layoutInfo, nullptr, &m_descriptorSetLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create global descriptor set layout!");
    }
}

void GlobalDescriptorSet::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes{
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, MAX_FRAMES_IN_FLIGHT * 4},
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    if(m_device->GetDevice().createDescriptorPool(&poolInfo, nullptr, &m_descriptorPool) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create global descriptor pool!");
    }
}

void GlobalDescriptorSet::createDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .pSetLayouts = layouts.data(),
    };

    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if(m_device->GetDevice().allocateDescriptorSets(&allocInfo, m_descriptorSets.data()) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate global descriptor sets!");
    }

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo cameraBufferInfo{
            .buffer = m_camera->m_buffers[i],
            .offset = 0,
            .range = m_camera->getBufferSize(),
        };
        vk::DescriptorBufferInfo lightsBufferInfo{
            .buffer = m_lights->m_buffers[i],
            .offset = 0,
            .range = m_lights->getBufferSize(),
        };
        vk::DescriptorBufferInfo clusterBufferInfo{
            .buffer = m_clusterBounds->m_buffers[i],
            .offset = 0,
            .range = m_clusterBounds->getBufferSize(),
        };
        vk::DescriptorBufferInfo lightGridInfo{
            .buffer = m_lightGrid->m_buffers[i],
            .offset = 0,
            .range = m_lightGrid->getBufferSize(),
        };
        vk::DescriptorBufferInfo lightIndicesInfo{
            .buffer = m_lightIndices->m_buffers[i],
            .offset = 0,
            .range = m_lightIndices->getBufferSize(),
        };

        std::array<vk::WriteDescriptorSet, 5> writes{
            vk::WriteDescriptorSet{
                .dstSet = m_descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &cameraBufferInfo,
            },
            vk::WriteDescriptorSet{
                .dstSet = m_descriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &lightsBufferInfo,
            },
            vk::WriteDescriptorSet{
                .dstSet = m_descriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &clusterBufferInfo,
            },
            vk::WriteDescriptorSet{
                .dstSet = m_descriptorSets[i],
                .dstBinding = 3,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &lightGridInfo,
            },
            vk::WriteDescriptorSet{
                .dstSet = m_descriptorSets[i],
                .dstBinding = 4,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &lightIndicesInfo,
            },
        };

        m_device->GetDevice().updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

} // namespace engine
