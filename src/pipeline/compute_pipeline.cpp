#include <engine/pipeline/compute_pipeline.h>

#include <array>

namespace engine
{

// TODO: this should take in shared ptr of globalSetLayout
PipelineCompute::PipelineCompute(std::shared_ptr<Device> device,
                                 const CreatePipelineParams& params,
                                 vk::DescriptorSetLayout globalSetLayout)
    : Pipeline(device, params)
    , m_globalSetLayout(globalSetLayout)
{
    createPipeline(params.m_shaderPaths);
}

PipelineCompute::~PipelineCompute() { }

void PipelineCompute::createPipeline(const ShaderCodePaths& paths)
{
    auto shaderCode = utils::readFileAsString(paths.m_computeShaderPath);

    vk::ShaderModule shaderModule = createShaderModule(shaderCode, shaderc_shader_kind::shaderc_compute_shader, paths.m_computeShaderPath);

    vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = shaderModule,
        .pName = "main",
    };

    std::array<vk::DescriptorSetLayout, 2> setLayouts = {m_globalSetLayout, m_descriptorSetLayout};
    vk::PipelineLayoutCreateInfo computeLayoutInfo{
        .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
        .pSetLayouts = setLayouts.data(),
    };

    if (m_device->GetDevice().createPipelineLayout(&computeLayoutInfo, nullptr, &m_pipelineLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error{ "Failed to create compute pipeline layout!" };
    }

    vk::ComputePipelineCreateInfo computeCreateInfo{
        .stage = shaderStageCreateInfo,
        .layout = m_pipelineLayout,
    };

    if (m_device->GetDevice().createComputePipelines(VK_NULL_HANDLE, 1, &computeCreateInfo, nullptr, &m_pipeline) != vk::Result::eSuccess)
    {
        throw std::runtime_error{ "Failed to create compute pipeline!" };
    }

    m_device->GetDevice().destroyShaderModule(shaderModule, nullptr);
}

} // namespace engine