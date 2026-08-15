#pragma once

#include <engine/pipeline/pipeline.h>

namespace engine
{

class PipelineShadowMapOmni : public Pipeline
{
public:
    explicit PipelineShadowMapOmni(std::shared_ptr<Device> device,
                                   std::shared_ptr<RenderPass> renderPass,
                                   const CreatePipelineParams& params);
    virtual ~PipelineShadowMapOmni();

private:
    void createPipeline(const ShaderCodePaths& paths) override;
};

} // namespace engine