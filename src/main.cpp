#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <chrono>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include "fullscreen_quad.h"
#include "include/camera.h"
#include "include/command_buffer.h"
#include "include/device.h"
#include "include/game_object.h"
#include "include/mesh.h"
#include "include/pipeline.h"
#include "include/renderer.h"
#include "include/swap_chain.h"
#include "include/texture.h"
#include "include/unifoms.h"
#include "include/user_interface.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

class EngineApplication
{
public:
    void run()
    {
        m_window = std::make_shared<engine::Window>();
        m_device = std::make_shared<engine::Device>(m_window);
        m_commandBuffer = std::make_shared<engine::CommandBuffer>(m_device);
        m_swapChain = std::make_shared<engine::SwapChain>(m_device, m_commandBuffer, m_window);
        m_camera = std::make_shared<engine::Camera>(m_window->Get(), m_swapChain);
        m_ui = std::make_shared<engine::UserInterface>(m_device, m_swapChain->GetRenderPass(engine::RenderPassStage::General));
        m_renderer = std::make_shared<engine::Renderer>(m_device, m_swapChain, m_commandBuffer, m_camera, m_ui);
        m_window->SetResizeCallback(engine::Renderer::framebufferResizeCallback);

        engine::Texture2DParams vikingTexParams{.m_filepath = "textures/viking_room.png"};
        engine::TextureCubeParams env_params{.m_filepaths = {
                                                 "textures/skybox1.jpg",
                                                 "textures/skybox2.jpg",
                                                 "textures/skybox3.jpg",
                                                 "textures/skybox4.jpg",
                                                 "textures/skybox5.jpg",
                                                 "textures/skybox6.jpg",
                                             }};

        auto cameraBinding = [&](engine::Uniform& uniform, int currentImage) {
            auto* camera = dynamic_cast<engine::UniformCamera*>(&uniform);
            if(camera)
            {
                engine::UniformCamera::UniformBufferObject ubo;
                glm::mat4 rayDir{1.0};
                rayDir = glm::translate(rayDir, m_camera->m_cameraPos);
                rayDir = glm::inverse(m_camera->m_proj * m_camera->m_view * rayDir);
                ubo.rayDir = rayDir;
                ubo.position = glm::vec4(m_camera->m_cameraPos, 1.0);
                camera->updateBuffer(&ubo, currentImage);
            }
        };

        auto lightBinding = [&](engine::Uniform& uniform, int currentImage) {
            auto* light = dynamic_cast<engine::UniformLight*>(&uniform);
            if(light)
            {
                engine::UniformLight::UniformBufferObject ubo{};
                ubo.position = glm::vec4(m_light_pos, 1.0f);
                ubo.powerDensity = glm::vec4(1.0f);
                glm::mat4 depthProjectionMatrix =
                    glm::perspective(glm::radians(90.0f), 1.0f, engine::Camera::zNear, engine::Camera::zFar);

                glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(ubo.position), m_light_facing, glm::vec3(0, 1, 0));
                depthProjectionMatrix[1][1] *= -1;
                ubo.shadowViewProj = depthProjectionMatrix * depthViewMatrix;
                light->updateBuffer(&ubo, currentImage);
            }
        };

        auto debug = std::make_unique<engine::FullscreenQuadMesh>(m_device, m_commandBuffer);
        m_testInterior = std::make_unique<engine::GameObject>(m_device, m_commandBuffer, "models/InteriorTest.obj");
        m_testInterior->addUniform<engine::UniformGameObject>(
            0, vk::ShaderStageFlagBits::eVertex, [&](engine::Uniform& uniform, int currentImage) {
                auto* mvp = dynamic_cast<engine::UniformGameObject*>(&uniform);
                if(mvp)
                {
                    engine::UniformGameObject::UniformBufferObject ubo{};
                    ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                    ubo.view = m_camera->m_view;
                    ubo.proj = m_camera->m_proj;
                    mvp->updateBuffer(&ubo, currentImage);
                }
            });
        m_testInterior->addTexture(1, vk::ShaderStageFlagBits::eFragment, std::move(vikingTexParams));
        m_testInterior->addUniform<engine::UniformLight>(
            2, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex, lightBinding);
        m_testInterior->addUniform<engine::UniformCamera>(3, vk::ShaderStageFlagBits::eFragment, cameraBinding);
        m_testInterior->finalizeGameObject(
            m_swapChain, {.m_vertexShaderPath = "shaders/transform.vert", .m_fragmentShaderPath = "shaders/max_blinn.frag"}, true);

        auto fsquad = std::make_unique<engine::FullscreenQuadMesh>(m_device, m_commandBuffer);
        m_skybox = std::make_unique<engine::GameObject>(m_device, m_commandBuffer, std::move(fsquad));
        m_skybox->addUniform<engine::UniformCamera>(0, vk::ShaderStageFlagBits::eVertex, cameraBinding);
        m_skybox->addTexture(1, vk::ShaderStageFlagBits::eFragment, std::move(env_params));
        m_skybox->finalizeGameObject(m_swapChain,
                                     {.m_vertexShaderPath = "shaders/env.vert", .m_fragmentShaderPath = "shaders/env.frag"});

        mainLoop();
    }

private:
    std::shared_ptr<engine::Window> m_window;
    std::shared_ptr<engine::Device> m_device;
    std::shared_ptr<engine::SwapChain> m_swapChain;
    std::shared_ptr<engine::CommandBuffer> m_commandBuffer;
    std::shared_ptr<engine::Renderer> m_renderer;
    std::shared_ptr<engine::UserInterface> m_ui;

    std::shared_ptr<engine::Camera> m_camera;

    std::unique_ptr<engine::GameObject> m_testInterior;
    std::unique_ptr<engine::GameObject> m_skybox;

    glm::vec3 m_light_pos = glm::vec3(-2.0f, 0.1f, 0.0f);
    glm::vec3 m_light_facing = glm::vec3(1.0f);

    void mainLoop()
    {
        auto prevTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();

        while(!glfwWindowShouldClose(m_window->Get()))
        {
            currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - prevTime).count();
            prevTime = currentTime;

            glfwPollEvents();
            m_camera->processInput(m_window->Get(), time);

            engine::UserInterfaceObjectReferences refs{};
            refs.m_pipelines = {m_testInterior->GetPipeline(), m_skybox->GetPipeline(), m_testInterior->GetShadowPipeline()};
            refs.m_light_pos = &m_light_pos;
            refs.m_light_facing = &m_light_facing;
            m_ui->buildInterface(refs);

            std::vector<engine::DrawFrameParams> params_list;
            params_list.push_back(m_skybox->getDrawFrameParams());
            params_list.push_back(m_testInterior->getDrawFrameParams());
            m_renderer->drawFrame(params_list);
        }

        vkDeviceWaitIdle(m_device->GetDevice());
    }
};

int main()
{
    EngineApplication app;

    try
    {
        app.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch(...)
    {
        std::cerr << "Unknown error" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}