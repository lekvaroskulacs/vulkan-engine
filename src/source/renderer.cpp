#include <engine/renderer/renderer.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <chrono>
#include <cstring>
#include <stdexcept>

namespace engine
{

Renderer::Renderer(std::shared_ptr<Device> device,
                   std::shared_ptr<SwapChain> swapChain,
                   RenderPassList renderPasses,
                   std::shared_ptr<CommandBuffer> commandBuffers,
                   std::shared_ptr<Camera> camera,
                   std::shared_ptr<UserInterface> ui,
                   std::shared_ptr<GlobalDescriptorSet> globalDescriptorSet)
    : m_device{device}
    , m_swapChain{swapChain}
    , m_renderPasses{std::move(renderPasses)}
    , m_commandBuffers{commandBuffers}
    , m_camera{camera}
    , m_ui{ui}
    , m_globalDescriptorSet{globalDescriptorSet}
{
    createSyncObjects();
}

Renderer::~Renderer()
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

void Renderer::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    app->m_framebufferResized = true;
}

void Renderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, const std::vector<DrawFrameData>& params_list)
{
    vk::CommandBufferBeginInfo beginInfo{
        .flags = {},
        .pInheritanceInfo = nullptr,
    };
    if(commandBuffer.begin(&beginInfo) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    for(auto& [stage, pass] : m_renderPasses)
    {
        auto clearValues = pass->GetClearValues();
        auto extent = pass->GetExtent();

        for(uint32_t instance = 0; instance < pass->GetInstanceCount(); ++instance)
        {
            vk::RenderPassBeginInfo renderPassInfo{
                .renderPass = pass->Get(),
                .framebuffer = pass->GetFrameBuffer(instance, imageIndex),
                .renderArea = {.offset = {0, 0}, .extent = extent},
                .clearValueCount = static_cast<uint32_t>(clearValues.size()),
                .pClearValues = clearValues.data(),
            };
            commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

            bool globalSetBound = false;

            for(const auto& drawable : params_list)
            {
                if(drawable.m_renderPassInfo.find(stage) == drawable.m_renderPassInfo.end())
                {
                    continue;
                }

                const auto& params = drawable.m_renderPassInfo.at(stage);

                if(auto pushConstants = pass->GetPushConstants(instance, drawable))
                {
                    commandBuffer.pushConstants(params.m_pipeline.GetLayout(),
                                                pushConstants->m_stage,
                                                0,
                                                sizeof(pushConstants->m_matrix),
                                                &pushConstants->m_matrix);
                }

                commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, params.m_pipeline.Get());

                if(stage == RenderPassStage::General && !globalSetBound)
                {
                    vk::DescriptorSet globalSet = m_globalDescriptorSet->GetDescriptorSet(m_currentFrame);
                    commandBuffer.bindDescriptorSets(
                        vk::PipelineBindPoint::eGraphics, params.m_pipeline.GetLayout(), 0, 1, &globalSet, 0, nullptr);
                    globalSetBound = true;
                }

                vk::Viewport viewport{
                    .x = 0.0f,
                    .y = 0.0f,
                    .width = static_cast<float>(extent.width),
                    .height = static_cast<float>(extent.height),
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
                };

                commandBuffer.setViewport(0, 1, &viewport);
                vk::Rect2D scissor{
                    .offset = {0, 0},
                    .extent = extent,
                };
                
                commandBuffer.setScissor(0, 1, &scissor);

                vk::Buffer vertexBuffers[] = {drawable.m_mesh.GetVertexBuffer()};
                vk::DeviceSize offsets[] = {0};
                commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
                commandBuffer.bindIndexBuffer(drawable.m_mesh.GetIndexBuffer(), 0, vk::IndexType::eUint32);
                uint32_t objectSetIndex = (stage == RenderPassStage::General) ? 1 : 0;
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                 params.m_pipeline.GetLayout(),
                                                 objectSetIndex,
                                                 1,
                                                 &params.m_pipeline.GetDescriptorSets()[m_currentFrame],
                                                 0,
                                                 nullptr);
                commandBuffer.drawIndexed(static_cast<uint32_t>(drawable.m_mesh.GetIndices().size()), 1, 0, 0, 0);
            }

            // ImGui only ever draws to screen, so it's tied to the General pass specifically.
            if(stage == RenderPassStage::General)
            {
                m_ui->renderInterface(commandBuffer);
            }

            commandBuffer.endRenderPass();
        }
    }

    commandBuffer.end();
}

void Renderer::drawFrame(const std::vector<DrawFrameData>& params_list)
{
    [[maybe_unused]] auto ignored = m_device->GetDevice().waitForFences(1, &m_inFlightFences[m_currentFrame], vk::True, UINT64_MAX);
    uint32_t imageIndex;
    vk::Result result = m_device->GetDevice().acquireNextImageKHR(
        m_swapChain->Get(), UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    if(result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChainResources();
        return;
    }
    else if(result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }
    ignored = m_device->GetDevice().resetFences(1, &m_inFlightFences[m_currentFrame]);
    m_commandBuffers->GetBuffers()[m_currentFrame].reset();

    recordCommandBuffer(m_commandBuffers->GetBuffers()[m_currentFrame], imageIndex, params_list);
    for(auto& params : params_list)
    {
        updateUniformBuffers(m_currentFrame, params);
    }

    vk::Semaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                           vk::PipelineStageFlagBits::eEarlyFragmentTests};

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

    if(m_device->GetGraphicsQueue().submit(1, &submitInfo, m_inFlightFences[m_currentFrame]) != vk::Result::eSuccess)
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

    if(result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_framebufferResized)
    {
        recreateSwapChainResources();
    }
    else if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::updateUniformBuffers(uint32_t currentImage, const DrawFrameData& params_list)
{
    for(auto& [_, perPass] : params_list.m_renderPassInfo)
    {
        for(auto& uniformParam : perPass.m_uniforms)
        {
            uniformParam.m_operation(uniformParam.m_uniform, currentImage);
        }
    }
}

void Renderer::recreateSwapChainResources()
{
    m_swapChain->recreateSwapChain();
    for(auto& [stage, pass] : m_renderPasses)
    {
        pass->recreate();
    }
}

void Renderer::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(m_swapChain->GetImages().size());
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    vk::SemaphoreCreateInfo semaphoreInfo{};
    vk::FenceCreateInfo fenceInfo{.flags = vk::FenceCreateFlagBits::eSignaled};
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if(m_device->GetDevice().createSemaphore(&semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != vk::Result::eSuccess ||
           m_device->GetDevice().createFence(&fenceInfo, nullptr, &m_inFlightFences[i]) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
    for(size_t i = 0; i < m_renderFinishedSemaphores.size(); i++)
    {
        if(m_device->GetDevice().createSemaphore(&semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

} // namespace engine
