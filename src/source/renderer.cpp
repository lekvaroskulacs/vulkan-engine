#include "../include/renderer.h"
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
                   std::shared_ptr<CommandBuffer> commandBuffers,
                   std::shared_ptr<Camera> camera,
                   std::shared_ptr<UserInterface> ui)
    : m_device{device}
    , m_swapChain{swapChain}
    , m_commandBuffers{commandBuffers}
    , m_camera{camera}
    , m_ui{ui}
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

void Renderer::updateShadowCubeFaces(uint32_t faceIndex,
                                     vk::CommandBuffer commandBuffer,
                                     const std::vector<DrawFrameParams>& params_list)
{
    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
    vk::Rect2D renderArea{
        .offset = {0, 0},
        .extent = {m_swapChain->m_shadowMapSize, m_swapChain->m_shadowMapSize},
    };

    vk::RenderPassBeginInfo renderPassInfo{
        .renderPass = m_swapChain->GetRenderPass(RenderPassStage::ShadowMap),
        .framebuffer = m_swapChain->GetFrameBuffers(RenderPassStage::ShadowMap)[faceIndex],
        .renderArea = renderArea,
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };

    auto lightPos = params_list[1].m_light_position;
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    switch(faceIndex)
    {
    case 0: // POSITIVE_X
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0));
        break;
    case 1: // NEGATIVE_X
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0));
        break;
    case 2: // POSITIVE_Y
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
        break;
    case 3: // NEGATIVE_Y
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));
        break;
    case 4: // POSITIVE_Z
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0));
        break;
    case 5: // NEGATIVE_Z
        viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0));
        break;
    }

    commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

    for(const auto& renderpassesParams : params_list)
    {
        if(renderpassesParams.m_renderPassInfo.find(RenderPassStage::ShadowMap) == renderpassesParams.m_renderPassInfo.end())
        {
            continue;
        }

        const auto& params = renderpassesParams.m_renderPassInfo.at(RenderPassStage::ShadowMap);

        //viewMatrix = glm::translate(viewMatrix, renderpassesParams.m_light_position);
        commandBuffer.pushConstants(
            params.m_pipeline.GetLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &viewMatrix);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, params.m_pipeline.Get());
        vk::Viewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(m_swapChain->m_shadowMapSize),
            .height = static_cast<float>(m_swapChain->m_shadowMapSize),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        commandBuffer.setViewport(0, 1, &viewport);
        vk::Rect2D scissor{
            .offset = {0, 0},
            .extent = {m_swapChain->m_shadowMapSize, m_swapChain->m_shadowMapSize},
        };
        commandBuffer.setScissor(0, 1, &scissor);

        vk::Buffer vertexBuffers[] = {renderpassesParams.m_mesh.GetVertexBuffer()};
        vk::DeviceSize offsets[] = {0};
        commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
        commandBuffer.bindIndexBuffer(renderpassesParams.m_mesh.GetIndexBuffer(), 0, vk::IndexType::eUint32);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         params.m_pipeline.GetLayout(),
                                         0,
                                         1,
                                         &params.m_pipeline.GetDescriptorSets()[m_currentFrame],
                                         0,
                                         nullptr);
        commandBuffer.drawIndexed(static_cast<uint32_t>(renderpassesParams.m_mesh.GetIndices().size()), 1, 0, 0, 0);
    }

    commandBuffer.endRenderPass();
}

void Renderer::recordCommandBuffer(vk::CommandBuffer commandBuffer,
                                   uint32_t imageIndex,
                                   const std::vector<DrawFrameParams>& params_list)
{
    vk::CommandBufferBeginInfo beginInfo{
        .flags = {},
        .pInheritanceInfo = nullptr,
    };
    if(commandBuffer.begin(&beginInfo) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    {
        for(uint32_t face = 0; face < 6; face++)
        {
            updateShadowCubeFaces(face, commandBuffer, params_list);
        }
    }

    // 2nd renderpass
    {
        std::array<vk::ClearValue, 2> clearValues{};
        clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
        clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
        vk::Rect2D renderArea{
            .offset = {0, 0},
            .extent = m_swapChain->GetExtent(),
        };

        vk::RenderPassBeginInfo renderPassInfo{
            .renderPass = m_swapChain->GetRenderPass(RenderPassStage::General),
            .framebuffer = m_swapChain->GetFrameBuffers(RenderPassStage::General)[imageIndex],
            .renderArea = renderArea,
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data(),
        };
        commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

        for(const auto& renderpassesParams : params_list)
        {
            const auto& params = renderpassesParams.m_renderPassInfo.at(RenderPassStage::General);

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, params.m_pipeline.Get());
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

            vk::Buffer vertexBuffers[] = {renderpassesParams.m_mesh.GetVertexBuffer()};
            vk::DeviceSize offsets[] = {0};

            commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
            commandBuffer.bindIndexBuffer(renderpassesParams.m_mesh.GetIndexBuffer(), 0, vk::IndexType::eUint32);
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                             params.m_pipeline.GetLayout(),
                                             0,
                                             1,
                                             &params.m_pipeline.GetDescriptorSets()[m_currentFrame],
                                             0,
                                             nullptr);
            commandBuffer.drawIndexed(static_cast<uint32_t>(renderpassesParams.m_mesh.GetIndices().size()), 1, 0, 0, 0);
        }

        m_ui->renderInterface(commandBuffer);

        commandBuffer.endRenderPass();
    }

    commandBuffer.end();
}

void Renderer::drawFrame(const std::vector<DrawFrameParams>& params_list)
{
    [[maybe_unused]] auto ignored = m_device->GetDevice().waitForFences(1, &m_inFlightFences[m_currentFrame], vk::True, UINT64_MAX);
    uint32_t imageIndex;
    vk::Result result = m_device->GetDevice().acquireNextImageKHR(
        m_swapChain->Get(), UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
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
        m_swapChain->recreateSwapChain();
    }
    else if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::updateUniformBuffers(uint32_t currentImage, const DrawFrameParams& params_list)
{
    for(auto& [_, perPass] : params_list.m_renderPassInfo)
    {
        for(auto& uniformParam : perPass.m_uniforms)
        {
            uniformParam.m_operation(uniformParam.m_uniform, currentImage);
        }
    }
}

void Renderer::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(m_swapChain->GetImages(RenderPassStage::General).size());
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
