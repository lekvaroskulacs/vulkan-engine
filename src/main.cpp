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

#include "include/command_buffer.h"
#include "include/descriptor_sets.h"
#include "include/device.h"
#include "include/mesh.h"
#include "include/pipeline.h"
#include "include/renderer.h"
#include "include/swap_chain.h"
#include "include/texture.h"
#include "include/unifoms.h"

class EngineApplication
{
public:
    void run()
    {
        m_window = std::make_shared<engine::Window>();
        m_device = std::make_shared<engine::Device>(m_window);
        m_swapChain = std::make_shared<engine::SwapChain>(m_device, m_window);
        m_commandBuffer = std::make_shared<engine::CommandBuffer>(m_device, m_swapChain);

        engine::TextureParams params{.m_filepath = "textures/viking_room.png"};
        m_texture = std::make_shared<engine::Texture>(m_device, m_swapChain, m_commandBuffer, params);
        m_viking_room = std::make_unique<engine::Mesh>(m_device, m_commandBuffer, engine::MODEL_PATH);
        m_uniforms = std::make_shared<engine::Uniform>(m_device);
        m_uniforms->createUniformBuffers<engine::UniformBufferObject>();
        std::vector<std::shared_ptr<engine::Uniform>> uniform_list = {m_uniforms};
        std::vector<std::shared_ptr<engine::Texture>> texture_list = {m_texture};
        m_pipeline = std::make_shared<engine::Pipeline>(m_device, m_swapChain, uniform_list, texture_list);
        m_descriptor_sets = std::make_shared<engine::DescriptorSets>(m_device, m_pipeline, uniform_list, texture_list);
        m_renderer = std::make_shared<engine::Renderer>(m_device, m_swapChain, m_commandBuffer);
        m_window->SetResizeCallback(engine::Renderer::framebufferResizeCallback);

        mainLoop();
    }

private:
    std::shared_ptr<engine::Window> m_window;
    std::shared_ptr<engine::Device> m_device;
    std::shared_ptr<engine::SwapChain> m_swapChain;
    std::shared_ptr<engine::Pipeline> m_pipeline;
    std::shared_ptr<engine::CommandBuffer> m_commandBuffer;
    std::shared_ptr<engine::Texture> m_texture;
    std::shared_ptr<engine::Uniform> m_uniforms;
    std::shared_ptr<engine::DescriptorSets> m_descriptor_sets;
    std::shared_ptr<engine::Renderer> m_renderer;

    std::unique_ptr<engine::Mesh> m_viking_room;

    void mainLoop()
    {
        while(!glfwWindowShouldClose(m_window->Get()))
        {
            glfwPollEvents();
            engine::DrawFrameParams params{.m_uniforms = *m_uniforms,
                                           .m_descriptorSets = *m_descriptor_sets,
                                           .m_pipeline = *m_pipeline,
                                           .m_mesh = *m_viking_room};
            m_renderer->drawFrame(params);
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