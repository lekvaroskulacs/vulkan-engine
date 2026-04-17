#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "swap_chain.h"
#include "utils.h"
#include "vertex.h"

namespace engine
{

class Pipeline
{
public:
    VkDescriptorSetLayout GetDescriptorSetLayout();
    VkPipeline Get() const;
    VkPipelineLayout GetLayout() const;

    explicit Pipeline(std::shared_ptr<Device> device, std::shared_ptr<SwapChain> swapChain);
    ~Pipeline();

private:
    void createDescriptorSetLayout();
    vk::ShaderModule createShaderModule(const std::vector<char>& code);
    void createGraphicsPipeline();

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;

    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;
};

} // namespace engine