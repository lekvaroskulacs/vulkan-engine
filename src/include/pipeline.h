#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "swap_chain.h"
#include "texture.h"
#include "unifoms.h"
#include "utils.h"
#include "vertex.h"

namespace engine
{

struct CreatePipelineParams
{
    std::string m_vertexShaderPath;
    std::string m_fragmentShaderPath;
};

class Pipeline
{
public:
    VkDescriptorSetLayout GetDescriptorSetLayout();
    VkPipeline Get() const;
    VkPipelineLayout GetLayout() const;

    explicit Pipeline(std::shared_ptr<Device> device,
                      std::shared_ptr<SwapChain> swapChain,
                      const std::vector<std::shared_ptr<Uniform>>& uniforms,
                      const std::vector<std::shared_ptr<Texture>>& textures,
                      const CreatePipelineParams& params);
    ~Pipeline();

private:
    void createDescriptorSetLayout(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                                   const std::vector<std::shared_ptr<Texture>>& textures);
    vk::ShaderModule createShaderModule(const std::vector<char>& code);
    void createGraphicsPipeline(const CreatePipelineParams& params);

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;

    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;
};

} // namespace engine