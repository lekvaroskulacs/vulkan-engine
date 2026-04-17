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

Pipeline::Pipeline(std::shared_ptr<Device> device, std::shared_ptr<SwapChain> swapChain)
    : m_device(device)
    , m_swapChain(swapChain)
{
    createDescriptorSetLayout();
    createGraphicsPipeline();
}

Pipeline::~Pipeline()
{
    m_device->GetDevice().destroyDescriptorSetLayout(m_descriptorSetLayout, nullptr);
    m_device->GetDevice().destroyPipeline(m_graphicsPipeline, nullptr);
    m_device->GetDevice().destroyPipelineLayout(m_pipelineLayout, nullptr);
}

void Pipeline::createDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding{.binding = 0,
                                                    .descriptorType =
                                                        vk::DescriptorType::eUniformBuffer,
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

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                              samplerLayoutBinding};
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    if(m_device->GetDevice().createDescriptorSetLayout(
           &layoutInfo, nullptr, &m_descriptorSetLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

vk::ShaderModule Pipeline::createShaderModule(const std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    vk::ShaderModule shaderModule;
    if(m_device->GetDevice().createShaderModule(&createInfo, nullptr, &shaderModule) !=
       vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

void Pipeline::createGraphicsPipeline()
{
    auto vertShaderCode = engine::utils::readFile("shaders/bin/vert.spv");
    auto fragShaderCode = engine::utils::readFile("shaders/bin/frag.spv");

    vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex, .module = vertShaderModule, .pName = "main"};

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = fragShaderModule, .pName = "main"};

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount =
                                                        static_cast<uint32_t>(dynamicStates.size()),
                                                    .pDynamicStates = dynamicStates.data()};

    auto bindingDesc = Vertex::getBindingDescription();
    auto attribDesc = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDesc,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size()),
        .pVertexAttributeDescriptions = attribDesc.data()};

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList, .primitiveRestartEnable = vk::False};

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

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .srcColorBlendFactor = vk::BlendFactor::eOne,
        .dstColorBlendFactor = vk::BlendFactor::eZero,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
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

    if(m_device->GetDevice().createPipelineLayout(
           &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != vk::Result::eSuccess)
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

    if(m_device->GetDevice().createGraphicsPipelines(
           {}, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    m_device->GetDevice().destroyShaderModule(vertShaderModule, nullptr);
    m_device->GetDevice().destroyShaderModule(fragShaderModule, nullptr);
}

} // namespace engine
