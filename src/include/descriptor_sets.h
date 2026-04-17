#pragma once

#include "pipeline.h"
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
                            const std::vector<std::shared_ptr<Uniform>>& uniforms,
                            const std::vector<std::shared_ptr<Texture>>& textures);
    ~DescriptorSets();

    void createDescriptor();
    void createDescriptorPool(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                              const std::vector<std::shared_ptr<Texture>>& textures);
    void createDescriptorSets(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                              const std::vector<std::shared_ptr<Texture>>& textures);

    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

private:
    std::shared_ptr<Device> m_device;
    std::shared_ptr<Pipeline> m_pipeline;
};

} // namespace engine
