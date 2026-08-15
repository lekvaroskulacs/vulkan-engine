#pragma once

#include <engine/pipeline/pipeline.h>
#include <engine/renderpass/render_pass.h>
#include <engine/camera/camera.h>
#include <engine/command_buffer/command_buffer.h>
#include <engine/global_descriptor_set/global_descriptor_set.h>
#include <engine/mesh/mesh.h>
#include <engine/swap_chain/swap_chain.h>
#include <engine/user_interface/user_interface.h>

namespace engine
{

/// Data per draw call
struct DrawFrameData
{
    struct PerRenderPassParams
    {
        struct UniformParam
        {
            UpdatableBuffer& m_uniform;
            std::function<void(UpdatableBuffer&, int)> m_operation;
        };
        std::vector<UniformParam> m_uniforms;
        Pipeline& m_pipeline;
    };

    std::unordered_map<RenderPassStage, PerRenderPassParams> m_renderPassInfo;
    Mesh& m_mesh;
    glm::vec3 m_shadow_light_position;
};

class Renderer
{
public:
    explicit Renderer(std::shared_ptr<Device> device,
                      std::shared_ptr<SwapChain> swapChain,
                      RenderPassList renderPasses,
                      std::shared_ptr<CommandBuffer> commandBuffers,
                      std::shared_ptr<Camera> camera,
                      std::shared_ptr<UserInterface> ui,
                      std::shared_ptr<GlobalDescriptorSet> globalDescriptorSet);
    ~Renderer();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, const std::vector<DrawFrameData>& params_list);

    void drawFrame(const std::vector<DrawFrameData>& params_list);
    void updateUniformBuffers(uint32_t currentImage, const DrawFrameData& params_list);

    uint32_t GetCurrentFrame() const
    {
        return m_currentFrame;
    }

private:
    void createSyncObjects();
    void recreateSwapChainResources();

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;
    RenderPassList m_renderPasses;
    std::shared_ptr<CommandBuffer> m_commandBuffers;
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<UserInterface> m_ui;
    std::shared_ptr<GlobalDescriptorSet> m_globalDescriptorSet;

    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_inFlightFences;

    bool m_framebufferResized = false;
    uint32_t m_currentFrame = 0;
};

} // namespace engine