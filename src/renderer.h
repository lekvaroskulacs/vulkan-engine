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
            vkDestroySemaphore(m_device->GetDevice(), m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_device->GetDevice(), m_inFlightFences[i], nullptr);
        }

        for(size_t i = 0; i < m_renderFinishedSemaphores.size(); i++)
        {
            vkDestroySemaphore(m_device->GetDevice(), m_renderFinishedSemaphores[i], nullptr);
        }
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto* app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
        app->m_framebufferResized = true;
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex,
                             const Pipeline& pipeline,
                             const Mesh& mesh,
                             const DescriptorSets& descriptorSets)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_swapChain->GetRenderPass();
        renderPassInfo.framebuffer = m_swapChain->GetFrameBuffers()[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapChain->GetExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Get());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapChain->GetExtent().width);
        viewport.height = static_cast<float>(m_swapChain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_swapChain->GetExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {mesh.GetVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, mesh.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline.GetLayout(),
                                0,
                                1,
                                &descriptorSets.m_descriptorSets[m_currentFrame],
                                0,
                                nullptr);
        vkCmdDrawIndexed(
            commandBuffer, static_cast<uint32_t>(mesh.GetIndices().size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }
    }

    void drawFrame(DrawFrameParams& params)
    {
        vkWaitForFences(
            m_device->GetDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_device->GetDevice(),
                                                m_swapChain->Get(),
                                                UINT64_MAX,
                                                m_imageAvailableSemaphores[m_currentFrame],
                                                VK_NULL_HANDLE,
                                                &imageIndex);

        if(result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_swapChain->recreateSwapChain();
            return;
        }
        else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        vkResetFences(m_device->GetDevice(), 1, &m_inFlightFences[m_currentFrame]);
        vkResetCommandBuffer(m_commandBuffers->GetBuffers()[m_currentFrame], 0);
        recordCommandBuffer(m_commandBuffers->GetBuffers()[m_currentFrame],
                            imageIndex,
                            params.m_pipeline,
                            params.m_mesh,
                            params.m_descriptorSets);

        updateUniformBuffer(m_currentFrame, params.m_uniforms);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = m_commandBuffers->GetBufferPtr(m_currentFrame);

        VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[imageIndex]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if(vkQueueSubmit(
               m_device->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) !=
           VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {m_swapChain->Get()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(m_device->GetPresentQueue(), &presentInfo);

        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
           m_framebufferResized)
        {
            m_swapChain->recreateSwapChain();
        }
        else if(result != VK_SUCCESS)
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

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if(vkCreateSemaphore(m_device->GetDevice(),
                                 &semaphoreInfo,
                                 nullptr,
                                 &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
               vkCreateFence(m_device->GetDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) !=
                   VK_SUCCESS)
            {

                throw std::runtime_error("Failed to create synchronization objects for a frame!");
            }
        }

        for(size_t i = 0; i < m_renderFinishedSemaphores.size(); i++)
        {
            if(vkCreateSemaphore(m_device->GetDevice(),
                                 &semaphoreInfo,
                                 nullptr,
                                 &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create synchronization objects for a frame!");
            }
        }
    }

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;
    std::shared_ptr<CommandBuffer> m_commandBuffers;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    bool m_framebufferResized = false;

    uint32_t m_currentFrame = 0;
};

} // namespace engine