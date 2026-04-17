#pragma once

#include "command_buffer.h"
#include "descriptor_sets.h"
#include "mesh.h"

namespace engine
{

struct DrawFrameParams
{
    Uniforms& m_uniforms;
    DescriptorSets& m_descriptorSets;
    Pipeline& m_pipeline;
    Mesh& m_mesh;
};

class Renderer
{
public:
    explicit Renderer(std::shared_ptr<Device> device,
                      std::shared_ptr<SwapChain> swapChain,
                      std::shared_ptr<CommandBuffer> commandBuffers);
    ~Renderer();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void recordCommandBuffer(vk::CommandBuffer commandBuffer,
                             uint32_t imageIndex,
                             const Pipeline& pipeline,
                             const Mesh& mesh,
                             const DescriptorSets& descriptorSets);

    void drawFrame(DrawFrameParams& params);
    void updateUniformBuffer(uint32_t currentImage, Uniforms& uniforms);

private:
    void createSyncObjects();

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;
    std::shared_ptr<CommandBuffer> m_commandBuffers;

    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_inFlightFences;

    bool m_framebufferResized = false;
    uint32_t m_currentFrame = 0;
};

} // namespace engine