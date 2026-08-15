#pragma once

#include <engine/pipeline/pipeline.h>

namespace engine
{

class PipelineShadowMap : public Pipeline
{
public:
    explicit PipelineShadowMap(std::shared_ptr<Device> device,
                               std::shared_ptr<RenderPass> renderPass,
                               const CreatePipelineParams& params);
    virtual ~PipelineShadowMap();

private:
    void createPipeline(const ShaderCodePaths& paths) override;
};

} // namespace engine