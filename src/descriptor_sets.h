#pragma once

#include "texture.h"
#include "unifoms.h"
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine
{

class DescriptorSets
{
public:
    explicit DescriptorSets(std::shared_ptr<Device> device,
                            std::shared_ptr<Pipeline> pipeline,
                            const Uniforms& uniforms,
                            const Texture& textures);
    ~DescriptorSets();

    void createDescriptor();
    void createDescriptorPool();
    void createDescriptorSets(const Uniforms& uniforms, const Texture& textures);

    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

private:
    std::shared_ptr<Device> m_device;
    std::shared_ptr<Pipeline> m_pipeline;
};

} // namespace engine
