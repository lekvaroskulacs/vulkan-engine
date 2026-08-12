#include "render_pipeline.h"

namespace engine
{

PipelineRender::PipelineRender(std::shared_ptr<Device> device,
                               std::shared_ptr<RenderPass> renderPass,
                               const CreatePipelineParams& params)
    : Pipeline(device, renderPass, params)
{
    createPipeline(params.m_shaderPaths);
}

PipelineRender::~PipelineRender() { }

void PipelineRender::createPipeline(const ShaderCodePaths& paths)
{
    auto vertShaderCode = engine::utils::readFileAsString(paths.m_vertexShaderPath);
    auto fragShaderCode = engine::utils::readFileAsString(paths.m_fragmentShaderPath);

    vk::ShaderModule vertShaderModule =
        createShaderModule(vertShaderCode, shaderc_shader_kind::shaderc_vertex_shader, paths.m_vertexShaderPath);
    vk::ShaderModule fragShaderModule =
        createShaderModule(fragShaderCode, shaderc_shader_kind::shaderc_fragment_shader, paths.m_fragmentShaderPath);

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
        .width = (float)m_renderPass->GetExtent().width,
        .height = (float)m_renderPass->GetExtent().height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D scissor{.offset = {0, 0}, .extent = m_renderPass->GetExtent()};

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
        .renderPass = m_renderPass->Get(),
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

} // namespace engine