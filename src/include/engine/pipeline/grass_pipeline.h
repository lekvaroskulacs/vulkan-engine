#pragma once

#include <engine/pipeline/pipeline.h>

namespace engine
{

class PipelineGrass : public Pipeline
{
public:
    explicit PipelineGrass(std::shared_ptr<Device> device,
                            std::shared_ptr<RenderPass> renderPass,
                            const CreatePipelineParams& params,
                            vk::DescriptorSetLayout globalSetLayout);
    virtual ~PipelineGrass();

private:
    void createPipeline(const ShaderCodePaths& paths) override;

    vk::DescriptorSetLayout m_globalSetLayout;
};

} // namespace engine