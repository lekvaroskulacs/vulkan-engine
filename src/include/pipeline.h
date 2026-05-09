#pragma once

#include <memory>
#include <shaderc/shaderc.hpp>
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
    vk::DescriptorPool GetDescriptorPool() const;
    std::vector<vk::DescriptorSet> GetDescriptorSets() const;

    explicit Pipeline(std::shared_ptr<Device> device,
                      std::shared_ptr<SwapChain> swapChain,
                      const std::vector<std::shared_ptr<Uniform>>& uniforms,
                      const std::vector<std::shared_ptr<Texture>>& textures,
                      const CreatePipelineParams& params);
    ~Pipeline();

    void recreatePipeline();

private:
    void createDescriptorSetLayout(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                                   const std::vector<std::shared_ptr<Texture>>& textures);
    vk::ShaderModule createShaderModule(const std::string& code, shaderc_shader_kind kind, const std::string& inputFile);
    void createGraphicsPipeline(const CreatePipelineParams& params);

    void createDescriptor();
    void createDescriptorPool(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                              const std::vector<std::shared_ptr<Texture>>& textures);
    void createDescriptorSets(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                              const std::vector<std::shared_ptr<Texture>>& textures);

    // Cached for recreation
    CreatePipelineParams m_creationParams;

    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;

    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;
};

} // namespace engine