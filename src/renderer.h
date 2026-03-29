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
                      std::shared_ptr<CommandBuffer> commandBuffers)
        : m_device{device}
        , m_swapChain{swapChain}
        , m_commandBuffers{commandBuffers}
    {
        createSyncObjects();
    }

    ~Renderer()
    {
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_device->GetDevice().destroySemaphore(m_imageAvailableSemaphores[i], nullptr);
            m_device->GetDevice().destroyFence(m_inFlightFences[i], nullptr);
        }

        for(size_t i = 0; i < m_renderFinishedSemaphores.size(); i++)
        {
            m_device->GetDevice().destroySemaphore(m_renderFinishedSemaphores[i], nullptr);
        }
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto* app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
        app->m_framebufferResized = true;
    }

    void recordCommandBuffer(vk::CommandBuffer commandBuffer,
                             uint32_t imageIndex,
                             const Pipeline& pipeline,
                             const Mesh& mesh,
                             const DescriptorSets& descriptorSets)
    {
        vk::CommandBufferBeginInfo beginInfo{
            .flags = {},
            .pInheritanceInfo = nullptr,
        };

        if(commandBuffer.begin(&beginInfo) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        std::array<vk::ClearValue, 2> clearValues{};
        clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
        clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

        vk::Rect2D renderArea{
            .offset = {0, 0},
            .extent = m_swapChain->GetExtent(),
        };
        vk::RenderPassBeginInfo renderPassInfo{
            .renderPass = m_swapChain->GetRenderPass(),
            .framebuffer = m_swapChain->GetFrameBuffers()[imageIndex],
            .renderArea = renderArea,
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data(),
        };

        commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.Get());

        vk::Viewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(m_swapChain->GetExtent().width),
            .height = static_cast<float>(m_swapChain->GetExtent().height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        commandBuffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor{
            .offset = {0, 0},
            .extent = m_swapChain->GetExtent(),
        };
        commandBuffer.setScissor(0, 1, &scissor);

        vk::Buffer vertexBuffers[] = {mesh.GetVertexBuffer()};
        vk::DeviceSize offsets[] = {0};
        commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

        commandBuffer.bindIndexBuffer(mesh.GetIndexBuffer(), 0, vk::IndexType::eUint32);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         pipeline.GetLayout(),
                                         0,
                                         1,
                                         &descriptorSets.m_descriptorSets[m_currentFrame],
                                         0,
                                         nullptr);
        commandBuffer.drawIndexed(static_cast<uint32_t>(mesh.GetIndices().size()), 1, 0, 0, 0);

        commandBuffer.endRenderPass();

        commandBuffer.end(); // Why is result lost with vulkan.hpp?
    }

    void drawFrame(DrawFrameParams& params)
    {
        [[maybe_unused]] auto ignored = m_device->GetDevice().waitForFences(
            1, &m_inFlightFences[m_currentFrame], vk::True, UINT64_MAX);

        uint32_t imageIndex;
        vk::Result result =
            m_device->GetDevice().acquireNextImageKHR(m_swapChain->Get(),
                                                      UINT64_MAX,
                                                      m_imageAvailableSemaphores[m_currentFrame],
                                                      VK_NULL_HANDLE,
                                                      &imageIndex);

        if(result == vk::Result::eErrorOutOfDateKHR)
        {
            m_swapChain->recreateSwapChain();
            return;
        }
        else if(result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        ignored = m_device->GetDevice().resetFences(1, &m_inFlightFences[m_currentFrame]);
        m_commandBuffers->GetBuffers()[m_currentFrame].reset();
        recordCommandBuffer(m_commandBuffers->GetBuffers()[m_currentFrame],
                            imageIndex,
                            params.m_pipeline,
                            params.m_mesh,
                            params.m_descriptorSets);

        updateUniformBuffer(m_currentFrame, params.m_uniforms);

        vk::Semaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::Semaphore signalSemaphores[] = {m_renderFinishedSemaphores[imageIndex]};

        vk::SubmitInfo submitInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = m_commandBuffers->GetBufferPtr(m_currentFrame),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signalSemaphores,
        };

        if(m_device->GetGraphicsQueue().submit(1, &submitInfo, m_inFlightFences[m_currentFrame]) !=
           vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        vk::SwapchainKHR swapChains[] = {m_swapChain->Get()};

        vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphores,
            .swapchainCount = 1,
            .pSwapchains = swapChains,
            .pImageIndices = &imageIndex,
            .pResults = nullptr,
        };

        result = m_device->GetPresentQueue().presentKHR(&presentInfo);

        if(result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR ||
           m_framebufferResized)
        {
            m_swapChain->recreateSwapChain();
        }
        else if(result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void updateUniformBuffer(uint32_t currentImage, Uniforms& uniforms)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time =
            std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime)
                .count();

        engine::UniformBufferObject ubo{};
        ubo.model =
            glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        ubo.view = glm::lookAt(
            glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        ubo.proj = glm::perspective(glm::radians(45.0f),
                                    m_swapChain->GetExtent().width /
                                        (float)m_swapChain->GetExtent().height,
                                    0.1f,
                                    10.0f);

        ubo.proj[1][1] *= -1; // in OpenGL y is flipped

        memcpy(uniforms.m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

private:
    void createSyncObjects()
    {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(m_swapChain->GetImages().size());
        m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        vk::SemaphoreCreateInfo semaphoreInfo{};

        vk::FenceCreateInfo fenceInfo{.flags = vk::FenceCreateFlagBits::eSignaled};

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if(m_device->GetDevice().createSemaphore(
                   &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) !=
                   vk::Result::eSuccess ||
               m_device->GetDevice().createFence(&fenceInfo, nullptr, &m_inFlightFences[i]) !=
                   vk::Result::eSuccess)
            {

                throw std::runtime_error("Failed to create synchronization objects for a frame!");
            }
        }

        for(size_t i = 0; i < m_renderFinishedSemaphores.size(); i++)
        {
            if(m_device->GetDevice().createSemaphore(
                   &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != vk::Result::eSuccess)
            {
                throw std::runtime_error("Failed to create synchronization objects for a frame!");
            }
        }
    }

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