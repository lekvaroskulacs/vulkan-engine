#include <engine/pipeline/pipeline.h>
#include <stdexcept>

namespace engine
{

VkDescriptorSetLayout Pipeline::GetDescriptorSetLayout()
{
    return m_descriptorSetLayout;
}

VkPipeline Pipeline::Get() const
{
    return m_graphicsPipeline;
}

VkPipelineLayout Pipeline::GetLayout() const
{
    return m_pipelineLayout;
}

vk::DescriptorPool Pipeline::GetDescriptorPool() const
{
    return m_descriptorPool;
}

// TODO: take image index as param
std::vector<vk::DescriptorSet> Pipeline::GetDescriptorSets() const
{
    return m_descriptorSets;
}

Pipeline::Pipeline(std::shared_ptr<Device> device, std::shared_ptr<RenderPass> renderPass, const CreatePipelineParams& params)
    : m_device(device)
    , m_renderPass(renderPass)
    , m_recreationParams(params.m_shaderPaths)
{
    createDescriptorSetLayout(params.m_resources);
    createDescriptorPool(params.m_resources);
    createDescriptorSets(params.m_resources);
}

Pipeline::~Pipeline()
{
    m_device->GetDevice().destroyDescriptorPool(m_descriptorPool, nullptr);
    m_device->GetDevice().destroyDescriptorSetLayout(m_descriptorSetLayout, nullptr);
    m_device->GetDevice().destroyPipeline(m_graphicsPipeline, nullptr);
    m_device->GetDevice().destroyPipelineLayout(m_pipelineLayout, nullptr);
}

void Pipeline::recreatePipeline()
{
    m_device->GetDevice().waitIdle();
    m_device->GetDevice().destroyPipeline(m_graphicsPipeline, nullptr);
    m_device->GetDevice().destroyPipelineLayout(m_pipelineLayout, nullptr);
    createPipeline(m_recreationParams);
}

void Pipeline::createDescriptorSetLayout(const std::unordered_map<uint8_t, PipelineResource>& resources)
{
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
    for(auto& [binding, resource] : resources)
    {
        if(std::holds_alternative<ConcreteBuffer*>(resource.m_resource))
        {
            vk::DescriptorSetLayoutBinding uboLayoutBinding{.binding = binding,
                                                            .descriptorType = vk::DescriptorType::eUniformBuffer,
                                                            .descriptorCount = 1,
                                                            .stageFlags = resource.m_stage,
                                                            .pImmutableSamplers = nullptr};
            layoutBindings.push_back(uboLayoutBinding);
        }
        else if(std::holds_alternative<Texture*>(resource.m_resource))
        {
            vk::DescriptorSetLayoutBinding samplerLayoutBinding{
                .binding = binding,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = 1,
                .stageFlags = resource.m_stage,
                .pImmutableSamplers = nullptr,
            };
            layoutBindings.push_back(samplerLayoutBinding);
        }
        //else if(std::holds_alternative<LightBuffer*>)
    }

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
        .pBindings = layoutBindings.data(),
    };

    if(m_device->GetDevice().createDescriptorSetLayout(&layoutInfo, nullptr, &m_descriptorSetLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

vk::ShaderModule Pipeline::createShaderModule(const std::string& code, shaderc_shader_kind kind, const std::string& inputFile)
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    shaderc::SpvCompilationResult shader = compiler.CompileGlslToSpv(code, kind, inputFile.c_str());
    if(shader.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cerr << shader.GetErrorMessage();
        throw std::runtime_error("Shader compilation error: " + shader.GetErrorMessage());
    }

    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = static_cast<size_t>(std::distance(shader.begin(), shader.end())) * sizeof(uint32_t),
        .pCode = reinterpret_cast<const uint32_t*>(shader.begin()),
    };

    vk::ShaderModule shaderModule;
    if(m_device->GetDevice().createShaderModule(&createInfo, nullptr, &shaderModule) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

void Pipeline::createDescriptorPool(const std::unordered_map<uint8_t, PipelineResource>& resources)
{
    uint32_t uniformCount = 0;
    uint32_t textureCount = 0;
    for(auto& [_, resource] : resources)
    {
        if(std::holds_alternative<ConcreteBuffer*>(resource.m_resource))
        {
            ++uniformCount;
        }
        else if(std::holds_alternative<Texture*>(resource.m_resource))
        {
            ++textureCount;
        }
    }

    std::vector<vk::DescriptorPoolSize> poolSizes{};

    if(uniformCount > 0)
    {
        poolSizes.push_back({vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT * uniformCount});
    }
    if(textureCount > 0)
    {
        poolSizes.push_back({vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT * textureCount});
    }

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

void Pipeline::createDescriptorSets(const std::unordered_map<uint8_t, PipelineResource>& resources)
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
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<vk::WriteDescriptorSet> descriptorWrites{};
        descriptorWrites.resize(resources.size());
        int writesPos = 0;

        std::vector<vk::DescriptorBufferInfo> bufferInfos;
        std::vector<vk::DescriptorImageInfo> imageInfos;
        bufferInfos.reserve(resources.size());
        imageInfos.reserve(resources.size());

        for(auto& [binding, resource] : resources)
        {
            descriptorWrites[writesPos].sType = vk::StructureType::eWriteDescriptorSet;
            descriptorWrites[writesPos].dstSet = m_descriptorSets[i];
            descriptorWrites[writesPos].dstBinding = binding;
            descriptorWrites[writesPos].dstArrayElement = 0;
            descriptorWrites[writesPos].descriptorCount = 1;

            if(std::holds_alternative<ConcreteBuffer*>(resource.m_resource))
            {
                auto& uniform = std::get<ConcreteBuffer*>(resource.m_resource);
                bufferInfos.push_back({
                    .buffer = uniform->m_buffers[i],
                    .offset = 0,
                    .range = uniform->getBufferSize(),
                });

                descriptorWrites[writesPos].descriptorType = vk::DescriptorType::eUniformBuffer;
                descriptorWrites[writesPos].pBufferInfo = &bufferInfos.back();
            }
            else if(std::holds_alternative<Texture*>(resource.m_resource))
            {
                auto& texture = std::get<Texture*>(resource.m_resource);
                auto flags = texture->GetAspectFlags();

                auto imageLayout = (flags & vk::ImageAspectFlagBits::eDepth) != vk::ImageAspectFlags{}
                                       ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                       : vk::ImageLayout::eShaderReadOnlyOptimal;
                imageInfos.push_back({
                    .sampler = texture->GetSampler(),
                    .imageView = texture->GetImageView(),
                    .imageLayout = imageLayout,
                });

                descriptorWrites[writesPos].descriptorType = vk::DescriptorType::eCombinedImageSampler;
                descriptorWrites[writesPos].pImageInfo = &imageInfos.back();
            }

            writesPos++;
        }

        m_device->GetDevice().updateDescriptorSets(
            static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

} // namespace engine
