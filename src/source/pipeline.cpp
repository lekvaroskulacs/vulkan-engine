#include "../include/pipeline.h"
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

std::vector<vk::DescriptorSet> Pipeline::GetDescriptorSets() const
{
    return m_descriptorSets;
}

Pipeline::Pipeline(std::shared_ptr<Device> device,
                   std::shared_ptr<SwapChain> swapChain,
                   const std::vector<std::shared_ptr<Uniform>>& uniforms,
                   const std::vector<std::shared_ptr<Texture>>& textures,
                   const CreatePipelineParams& params)
    : m_device(device)
    , m_swapChain(swapChain)
    , m_creationParams(params)
{
    createDescriptorSetLayout(uniforms, textures);
    createGraphicsPipeline(params);
    createDescriptorPool(uniforms, textures);
    createDescriptorSets(uniforms, textures);
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
    createGraphicsPipeline(m_creationParams);
}

// TODO: maybe provide uniforms and textures in <int, Uniform> map, corresponding to binding
void Pipeline::createDescriptorSetLayout(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                                         const std::vector<std::shared_ptr<Texture>>& textures)
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding{.binding = 0,
                                                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                                                    .descriptorCount = 1,
                                                    .stageFlags = vk::ShaderStageFlagBits::eVertex,
                                                    .pImmutableSamplers = nullptr};

    vk::DescriptorSetLayoutBinding samplerLayoutBinding{
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
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

void Pipeline::createGraphicsPipeline(const CreatePipelineParams& params)
{
    auto vertShaderCode = engine::utils::readFileAsString(params.m_vertexShaderPath);
    auto fragShaderCode = engine::utils::readFileAsString(params.m_fragmentShaderPath);

    vk::ShaderModule vertShaderModule =
        createShaderModule(vertShaderCode, shaderc_shader_kind::shaderc_vertex_shader, params.m_vertexShaderPath);
    vk::ShaderModule fragShaderModule =
        createShaderModule(fragShaderCode, shaderc_shader_kind::shaderc_fragment_shader, params.m_fragmentShaderPath);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex, .module = vertShaderModule, .pName = "main"};

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = fragShaderModule, .pName = "main"};

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                                                    .pDynamicStates = dynamicStates.data()};

    auto bindingDesc = Vertex::getBindingDescription();
    auto attribDesc = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{.vertexBindingDescriptionCount = 1,
                                                           .pVertexBindingDescriptions = &bindingDesc,
                                                           .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size()),
                                                           .pVertexAttributeDescriptions = attribDesc.data()};

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList,
                                                           .primitiveRestartEnable = vk::False};

    vk::Viewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)m_swapChain->GetExtent().width,
        .height = (float)m_swapChain->GetExtent().height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D scissor{.offset = {0, 0}, .extent = m_swapChain->GetExtent()};

    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .scissorCount = 1,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = vk::False,
        .alphaToOneEnable = vk::False,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{.blendEnable = vk::False,
                                                               .srcColorBlendFactor = vk::BlendFactor::eOne,
                                                               .dstColorBlendFactor = vk::BlendFactor::eZero,
                                                               .colorBlendOp = vk::BlendOp::eAdd,
                                                               .srcAlphaBlendFactor = vk::BlendFactor::eOne,
                                                               .dstAlphaBlendFactor = vk::BlendFactor::eZero,
                                                               .alphaBlendOp = vk::BlendOp::eAdd,
                                                               .colorWriteMask =
                                                                   vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                                   vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 1,
        .pSetLayouts = &m_descriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    if(m_device->GetDevice().createPipelineLayout(&pipelineLayoutInfo, nullptr, &m_pipelineLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = m_pipelineLayout,
        .renderPass = m_swapChain->GetRenderPass(),
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    if(m_device->GetDevice().createGraphicsPipelines({}, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    m_device->GetDevice().destroyShaderModule(vertShaderModule, nullptr);
    m_device->GetDevice().destroyShaderModule(fragShaderModule, nullptr);
}

void Pipeline::createDescriptorPool(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                                    const std::vector<std::shared_ptr<Texture>>& textures)
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * uniforms.size());
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * textures.size());

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

void Pipeline::createDescriptorSets(const std::vector<std::shared_ptr<Uniform>>& uniforms,
                                    const std::vector<std::shared_ptr<Texture>>& textures)
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
        descriptorWrites.resize(uniforms.size() + textures.size());
        int writesPos = 0;
        for(size_t j = 0; j < uniforms.size(); j++)
        {
            vk::DescriptorBufferInfo bufferInfo{
                .buffer = uniforms[j]->m_uniformBuffers[i],
                .offset = 0,
                .range = sizeof(engine::UniformBufferObject),
            };

            descriptorWrites[writesPos].sType = vk::StructureType::eWriteDescriptorSet;
            descriptorWrites[writesPos].dstSet = m_descriptorSets[i];
            descriptorWrites[writesPos].dstBinding = writesPos;
            descriptorWrites[writesPos].dstArrayElement = 0;
            descriptorWrites[writesPos].descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrites[writesPos].descriptorCount = 1;
            descriptorWrites[writesPos].pBufferInfo = &bufferInfo;

            writesPos++;
        }

        for(size_t j = 0; j < textures.size(); j++)
        {
            vk::DescriptorImageInfo imageInfo{
                .sampler = textures[j]->GetSampler(),
                .imageView = textures[j]->GetImageView(),
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            };

            descriptorWrites[writesPos].sType = vk::StructureType::eWriteDescriptorSet;
            descriptorWrites[writesPos].dstSet = m_descriptorSets[i];
            descriptorWrites[writesPos].dstBinding = writesPos;
            descriptorWrites[writesPos].dstArrayElement = 0;
            descriptorWrites[writesPos].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptorWrites[writesPos].descriptorCount = 1;
            descriptorWrites[writesPos].pImageInfo = &imageInfo;

            writesPos++;
        }

        m_device->GetDevice().updateDescriptorSets(
            static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

} // namespace engine
