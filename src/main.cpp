#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
        m_camera = std::make_shared<engine::Camera>(m_window->Get());
        m_device = std::make_shared<engine::Device>(m_window);
        m_swapChain = std::make_shared<engine::SwapChain>(m_device, m_window);
        m_commandBuffer = std::make_shared<engine::CommandBuffer>(m_device);
        m_ui = std::make_shared<engine::UserInterface>(m_device, m_swapChain->GetRenderPass());

        engine::Texture2DParams params{.m_filepath = "textures/viking_room.png"};
        m_texture = std::make_shared<engine::Texture>(m_device, m_commandBuffer, params);
        m_viking_room = std::make_unique<engine::Mesh>(m_device, m_commandBuffer, engine::MODEL_PATH);

        engine::Texture2DParams params2{.m_filepath = "textures/Skull.jpg"};
        m_skull_tex = std::make_shared<engine::Texture>(m_device, m_commandBuffer, params2);
        m_skull = std::make_unique<engine::Mesh>(m_device, m_commandBuffer, "models/skull.obj");

        engine::TextureCubeParams env_params{.m_filepaths = {
                                                 "textures/skybox1.jpg",
                                                 "textures/skybox2.jpg",
                                                 "textures/skybox3.jpg",
                                                 "textures/skybox4.jpg",
                                                 "textures/skybox5.jpg",
                                                 "textures/skybox6.jpg",
                                             }};
        m_envTex = std::make_shared<engine::Texture>(m_device, m_commandBuffer, env_params);
        m_cube = std::make_unique<engine::Mesh>(m_device, m_commandBuffer, "models/cube.obj");
        m_fullscreen_quad = std::make_unique<engine::FullscreenQuadMesh>(m_device, m_commandBuffer);

        m_uniforms = std::make_shared<engine::Uniform>(m_device);
        // jo lenne mar creationkor eldonteni a tipusat, felesleges hivas
        m_uniforms->setBufferObject<engine::UniformBufferObject>();

        std::vector<std::shared_ptr<engine::Uniform>> uniform_list = {m_uniforms};
        std::vector<std::shared_ptr<engine::Texture>> texture_list = {m_texture};
        engine::CreatePipelineParams pipelineParams{.m_vertexShaderPath = "shaders/triangle.vert",
                                                    .m_fragmentShaderPath = "shaders/triangle.frag"};
        m_pipeline = std::make_unique<engine::Pipeline>(m_device, m_swapChain, uniform_list, texture_list, pipelineParams);

        std::vector<std::shared_ptr<engine::Texture>> texture_list_skull = {m_skull_tex};
        m_skull_pipeline = std::make_unique<engine::Pipeline>(m_device, m_swapChain, uniform_list, texture_list_skull, pipelineParams);

        std::vector<std::shared_ptr<engine::Texture>> texture_list_env = {m_envTex};
        pipelineParams.m_vertexShaderPath = "shaders/env.vert";
        pipelineParams.m_fragmentShaderPath = "shaders/env.frag";
        m_env_pipeline = std::make_unique<engine::Pipeline>(m_device, m_swapChain, uniform_list, texture_list_env, pipelineParams);

        m_renderer = std::make_shared<engine::Renderer>(m_device, m_swapChain, m_commandBuffer, m_camera, m_ui);
        m_window->SetResizeCallback(engine::Renderer::framebufferResizeCallback);

        mainLoop();
    }

private:
    std::shared_ptr<engine::Window> m_window;
    std::shared_ptr<engine::Device> m_device;
    std::shared_ptr<engine::SwapChain> m_swapChain;
    std::shared_ptr<engine::CommandBuffer> m_commandBuffer;
    std::shared_ptr<engine::Uniform> m_uniforms;
    std::shared_ptr<engine::Renderer> m_renderer;
    std::shared_ptr<engine::UserInterface> m_ui;

    std::unique_ptr<engine::Pipeline> m_pipeline;
    std::shared_ptr<engine::Texture> m_texture;
    std::unique_ptr<engine::Mesh> m_viking_room;

    std::unique_ptr<engine::Pipeline> m_skull_pipeline;
    std::shared_ptr<engine::Texture> m_skull_tex;
    std::unique_ptr<engine::Mesh> m_skull;

    std::unique_ptr<engine::Pipeline> m_env_pipeline;
    std::shared_ptr<engine::Texture> m_envTex;
    std::unique_ptr<engine::Mesh> m_cube;
    std::unique_ptr<engine::Mesh> m_fullscreen_quad;

    std::shared_ptr<engine::Camera> m_camera;

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
            refs.m_pipelines = {*m_pipeline, *m_env_pipeline, *m_skull_pipeline};
            m_ui->buildInterface(refs);

            std::vector<engine::DrawFrameParams> params_list;
            engine::DrawFrameParams params{.m_uniforms = *m_uniforms, .m_pipeline = *m_pipeline, .m_mesh = *m_viking_room};
            engine::DrawFrameParams sky{.m_uniforms = *m_uniforms, .m_pipeline = *m_env_pipeline, .m_mesh = *m_fullscreen_quad};
            engine::DrawFrameParams skull{.m_uniforms = *m_uniforms, .m_pipeline = *m_skull_pipeline, .m_mesh = *m_skull};
            params_list.push_back(params);
            params_list.push_back(sky);
            params_list.push_back(skull);
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