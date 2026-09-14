#pragma once

#include <engine/device/device.h>
#include <engine/lights/lights.h>
#include <engine/pipeline/pipeline.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <vulkan/vulkan.hpp>

#include <string>

namespace engine
{

class Renderer;

struct UserInterfaceObjectReferences
{
    std::unordered_map<std::string, Pipeline*> m_pipelines;
    glm::vec3* m_light_pos;
    glm::vec3* m_light_facing;
    std::vector<Light>* m_lights;
    Renderer* m_renderer;
};

class UserInterface
{
public:
    explicit UserInterface(std::shared_ptr<Device> device, vk::RenderPass renderpass);
    ~UserInterface();

    void renderInterface(vk::CommandBuffer commandBuffer);
    void buildInterface(const UserInterfaceObjectReferences& refs);

private:
    std::shared_ptr<Device> m_device;
    VkDescriptorPool m_imgui_descriptor_pool;
};

} // namespace engine
