#pragma once

#include <memory>
#include <shaderc/shaderc.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "../texture/texture.h"
#include "swap_chain.h"
#include "unifoms.h"
#include "utils.h"
#include "vertex.h"

namespace engine
{

using Resource = std::variant<Uniform*, Texture*>;
struct PipelineResource
{
    vk::ShaderStageFlags m_stage;
    Resource m_resource; /// DO NOT take ownership of this object! It may be deleted during runtime.
};

struct ShaderCodePaths
{
    std::string m_vertexShaderPath;
    std::string m_fragmentShaderPath;
};

struct CreatePipelineParams
{
    ShaderCodePaths m_shaderPaths;
    std::unordered_map<uint8_t, PipelineResource> m_resources;
};

class Pipeline
{
public:
    VkDescriptorSetLayout GetDescriptorSetLayout();
    VkPipeline Get() const;
    VkPipelineLayout GetLayout() const;
    vk::DescriptorPool GetDescriptorPool() const;
    std::vector<vk::DescriptorSet> GetDescriptorSets() const;

    explicit Pipeline(std::shared_ptr<Device> device, std::shared_ptr<SwapChain> swapChain, const CreatePipelineParams& params);
    virtual ~Pipeline();

    void recreatePipeline();

protected:
    void createDescriptorSetLayout(const std::unordered_map<uint8_t, PipelineResource>& resources);
    vk::ShaderModule createShaderModule(const std::string& code, shaderc_shader_kind kind, const std::string& inputFile);
    virtual void createPipeline(const ShaderCodePaths& paths) = 0;
    void createDescriptorPool(const std::unordered_map<uint8_t, PipelineResource>& resources);
    void createDescriptorSets(const std::unordered_map<uint8_t, PipelineResource>& resources);

    // Cached for recreation
    ShaderCodePaths m_recreationParams;

    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;

    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;
};

} // namespace engine