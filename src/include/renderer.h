#pragma once

#include "camera.h"
#include "command_buffer.h"
#include "descriptor_sets.h"
#include "mesh.h"
#include "user_interface.h"

namespace engine
{

/// Data per draw call
struct DrawFrameParams
{
    Uniform& m_uniforms;
    DescriptorSets& m_descriptorSets;
    Pipeline& m_pipeline;
    Mesh& m_mesh;
};

class Renderer
{
public:
    explicit Renderer(std::shared_ptr<Device> device,
                      std::shared_ptr<SwapChain> swapChain,
                      std::shared_ptr<CommandBuffer> commandBuffers,
                      std::shared_ptr<Camera> camera,
                      std::shared_ptr<UserInterface> ui);
    ~Renderer();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, const std::vector<DrawFrameParams>& params_list);

    void drawFrame(const std::vector<DrawFrameParams>& params_list);
    void updateUniformBuffer(uint32_t currentImage, Uniform& uniforms);

private:
    void createSyncObjects();

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;
    std::shared_ptr<CommandBuffer> m_commandBuffers;
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<UserInterface> m_ui;

    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_inFlightFences;

    bool m_framebufferResized = false;
    uint32_t m_currentFrame = 0;
};

} // namespace engine