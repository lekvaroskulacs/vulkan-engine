#pragma once

#include "device.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "pipeline.h"

#include <vulkan/vulkan.hpp>

namespace engine
{

struct UserInterfaceObjectReferences
{
    std::vector<std::reference_wrapper<Pipeline>> m_pipelines;
};

class UserInterface
{
public:
    explicit UserInterface(std::shared_ptr<Device> device, vk::RenderPass renderpass)
        : m_device{device}
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        {
            VkDescriptorPoolSize pool_sizes[] = {
                {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE},
                {VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE},
            };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 0;
            for(VkDescriptorPoolSize& pool_size : pool_sizes)
                pool_info.maxSets += pool_size.descriptorCount;
            pool_info.poolSizeCount = (uint32_t)IM_COUNTOF(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;
            [[maybe_unused]] auto err = vkCreateDescriptorPool(m_device->GetDevice(), &pool_info, nullptr, &m_imgui_descriptor_pool);
        }

        m_device->initImGui(m_imgui_descriptor_pool, renderpass);
    }

    ~UserInterface()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(m_device->GetDevice(), m_imgui_descriptor_pool, nullptr);
    }

    void renderInterface(vk::CommandBuffer commandBuffer)
    {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    void buildInterface(const UserInterfaceObjectReferences& refs)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();

        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Controls");

            if(ImGui::Button("Reload Shaders"))
            {
                for(engine::Pipeline& pipeline : refs.m_pipelines)
                {
                    pipeline.recreatePipeline();
                }
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }
    }

private:
    std::shared_ptr<Device> m_device;
    VkDescriptorPool m_imgui_descriptor_pool;
};

} // namespace engine
