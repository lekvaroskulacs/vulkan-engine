#pragma once

#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "../include/device.h"
#include "../texture/texture.h"

namespace engine
{

struct DrawFrameData;

enum class RenderPassStage
{
    General,
    ShadowMap,
    ShadowMapOmni
};

struct PushConstantData
{
    vk::ShaderStageFlags m_stage;
    glm::mat4 m_matrix;
};

class RenderPass
{
public:
    vk::RenderPass Get() const;

    virtual vk::Extent2D GetExtent() const = 0;
    virtual uint32_t GetInstanceCount() const;
    virtual vk::Framebuffer GetFrameBuffer(uint32_t instance, uint32_t imageIndex) const = 0;
    virtual std::vector<vk::ClearValue> GetClearValues() const = 0;
    virtual Texture* GetRenderTarget(uint32_t index = 0) const;
    virtual std::optional<PushConstantData> GetPushConstants(uint32_t instance, const DrawFrameData& frame) const;

    explicit RenderPass(std::shared_ptr<Device> device);
    virtual ~RenderPass();

    /// Rebuild any resources that depend on the swap chain (extent, image views). No-op by default.
    virtual void recreate();

protected:
    std::shared_ptr<Device> m_device;
    vk::RenderPass m_renderPass;
};

using RenderPassList = std::vector<std::pair<RenderPassStage, std::shared_ptr<RenderPass>>>;
const std::shared_ptr<RenderPass>& findRenderPass(const RenderPassList& passes, RenderPassStage stage);

} // namespace engine
