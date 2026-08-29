#include <engine/renderpass/render_pass.h>
#include <stdexcept>

namespace engine
{

vk::RenderPass RenderPass::Get() const
{
    return m_renderPass;
}

uint32_t RenderPass::GetInstanceCount() const
{
    return 1;
}

Texture* RenderPass::GetRenderTarget(uint32_t index) const
{
    throw std::runtime_error("This render pass doesn't expose a sampleable render target!");
}

std::optional<PushConstantData> RenderPass::GetPushConstants(uint32_t instance, const PerMeshRenderData& frame) const
{
    return std::nullopt;
}

RenderPass::RenderPass(std::shared_ptr<Device> device)
    : m_device{device}
{ }

RenderPass::~RenderPass()
{
    m_device->GetDevice().destroyRenderPass(m_renderPass, nullptr);
}

void RenderPass::recreate() { }

const std::shared_ptr<RenderPass>& findRenderPass(const RenderPassList& passes, RenderPassStage stage)
{
    for(auto& [passStage, pass] : passes)
    {
        if(passStage == stage)
        {
            return pass;
        }
    }

    throw std::runtime_error("No render pass registered for the requested stage!");
}

} // namespace engine
