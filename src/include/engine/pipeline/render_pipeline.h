#pragma once

#include <engine/pipeline/pipeline.h>

namespace engine
{

class PipelineRender : public Pipeline
{
public:
    explicit PipelineRender(std::shared_ptr<Device> device,
                            std::shared_ptr<RenderPass> renderPass,
                            const CreatePipelineParams& params,
                            vk::DescriptorSetLayout globalSetLayout);
    virtual ~PipelineRender();

private:
    void createPipeline(const ShaderCodePaths& paths) override;

    vk::DescriptorSetLayout m_globalSetLayout;
};

} // namespace engine