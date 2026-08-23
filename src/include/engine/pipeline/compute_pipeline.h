#pragma once

#include <engine/pipeline/pipeline.h>

namespace engine
{

class PipelineCompute : public Pipeline
{
public:
    explicit PipelineCompute(std::shared_ptr<Device> device,
                            const CreatePipelineParams& params,
                            vk::DescriptorSetLayout globalSetLayout);
    virtual ~PipelineCompute();

private:
    void createPipeline(const ShaderCodePaths& paths) override;

    vk::DescriptorSetLayout m_globalSetLayout;
};

} // namespace engine