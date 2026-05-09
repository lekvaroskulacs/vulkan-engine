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

    for(auto& params : params_list)
    {
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

        vk::Buffer vertexBuffers[] = {params.m_mesh.GetVertexBuffer()};
        vk::DeviceSize offsets[] = {0};

        commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
        commandBuffer.bindIndexBuffer(params.m_mesh.GetIndexBuffer(), 0, vk::IndexType::eUint32);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         params.m_pipeline.GetLayout(),
                                         0,
                                         1,
                                         &params.m_pipeline.GetDescriptorSets()[m_currentFrame],
                                         0,
                                         nullptr);
        commandBuffer.drawIndexed(static_cast<uint32_t>(params.m_mesh.GetIndices().size()), 1, 0, 0, 0);
    }

    m_ui->renderInterface(commandBuffer);

    commandBuffer.endRenderPass();
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
    updateUniformBuffer(m_currentFrame, params_list[0].m_uniforms);

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

void Renderer::updateUniformBuffer(uint32_t currentImage, Uniform& uniforms)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    engine::UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(m_camera->m_cameraPos, m_camera->m_cameraPos + m_camera->m_cameraFront, m_camera->m_cameraUp);
    ubo.proj =
        glm::perspective(glm::radians(45.0f), m_swapChain->GetExtent().width / (float)m_swapChain->GetExtent().height, 0.1f, 1000.0f);
    ubo.proj[1][1] *= -1;

    glm::mat4 rayDir{1.0};
    rayDir = glm::translate(rayDir, m_camera->m_cameraPos);
    rayDir = glm::inverse(ubo.proj * ubo.view * rayDir);
    ubo.rayDir = rayDir;
    // memcpy(uniforms.m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    m_device->copyMemoryToAllocation(&ubo, uniforms.m_uniformBuffersAllocations[currentImage], sizeof(ubo));
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
