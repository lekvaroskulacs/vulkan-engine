#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <engine/camera/camera.h>
#include <engine/device/device.h>
#include <engine/lights/lights.h>
#include <engine/uniforms/uniforms.h>

namespace engine
{

/// Holds resources bound once per frame (camera, scene lights) in a single descriptor set,
/// shared across every PipelineRender instance instead of being duplicated per GameObject.
class GlobalDescriptorSet
{
public:
    explicit GlobalDescriptorSet(std::shared_ptr<Device> device);
    ~GlobalDescriptorSet();

    vk::DescriptorSetLayout GetLayout() const;
    vk::DescriptorSet GetDescriptorSet(uint32_t frameIndex) const;

    void updateCamera(uint32_t frameIndex, const Camera& camera);
    void updateLights(uint32_t frameIndex, const std::vector<Light>& lights);

private:
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();

    std::shared_ptr<Device> m_device;
    std::unique_ptr<UniformCamera> m_camera; // binding 0
    std::unique_ptr<LightBuffer> m_lights;   // binding 1
    std::unique_ptr<ClusterBoundsBuffer> m_clusterBounds; // binding 2
    std::unique_ptr<LightGridBuffer> m_lightGrid; // binding 3
    std::unique_ptr<LightIndexBuffer> m_lightIndices; // binding 4

    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;
};

} // namespace engine
