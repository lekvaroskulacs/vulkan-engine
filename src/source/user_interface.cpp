#include <engine/user_interface/user_interface.h>
#include <engine/renderer/renderer.h>

namespace engine
{

UserInterface::UserInterface(std::shared_ptr<Device> device, vk::RenderPass renderpass)
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

UserInterface::~UserInterface()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(m_device->GetDevice(), m_imgui_descriptor_pool, nullptr);
}

void UserInterface::renderInterface(vk::CommandBuffer commandBuffer)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void UserInterface::buildInterface(const UserInterfaceObjectReferences& refs)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();

    {
        constexpr ImGuiTreeNodeFlags openByDefault = ImGuiTreeNodeFlags_DefaultOpen;

        ImGui::Begin("Menu");

        if(ImGui::CollapsingHeader("Shaders", openByDefault))
        {
            if(ImGui::Button("Reload Shaders"))
            {
                for(auto& [_, pipeline] : refs.m_pipelines)
                {
                    pipeline->recreatePipeline();
                }
            }
        }

        if(ImGui::CollapsingHeader("Lighting", openByDefault))
        {
            static bool clusteredShading = false;
            if(ImGui::Checkbox("Use clustered shading", &clusteredShading))
            {
                ShaderCodePaths shaders{.m_vertexShaderPath = "shaders/transform.vert"};
                if(clusteredShading)
                {
                    shaders.m_fragmentShaderPath = "shaders/clustered_lights.frag";
                }
                else
                {
                    shaders.m_fragmentShaderPath = "shaders/omni_shadow.frag";
                }
                auto interior = refs.m_pipelines.at("interior");
                interior->changeShadersRuntime(shaders);
            }

            if(ImGui::SliderFloat3("Shadow casting light position", &refs.m_light_pos->x, -4.0f, 4.0f))
            {
                // if value changed
            }

            // if(ImGui::SliderFloat3("Light target point", &refs.m_light_facing->x, -4.0f, 4.0f))
            // {
            //     // if value changed
            // }

            if (ImGui::CollapsingHeader("Light position controls"))
            {
                int lightId = 0;
                for(auto& light : *refs.m_lights)
                {
                    std::string label = "Light " + std::to_string(lightId++);
                    if(ImGui::SliderFloat3(label.c_str(), &light.position.x, -100.0f, 100.0f))
                    {
                        ;
                    }
                }
            }
        }

        if(ImGui::CollapsingHeader("Debug", openByDefault))
        {
            static bool displayClusters = false;
            if(ImGui::Checkbox("Display clusters", &displayClusters))
            {
                ShaderCodePaths shaders{.m_vertexShaderPath = "shaders/transform.vert"};
                if(displayClusters)
                {
                    shaders.m_fragmentShaderPath = "shaders/compute/debug_cluster.frag";
                }
                else
                {
                    shaders.m_fragmentShaderPath = "shaders/omni_shadow.frag";
                }
                auto interior = refs.m_pipelines.at("interior");
                interior->changeShadersRuntime(shaders);
            }
        }

        if(ImGui::CollapsingHeader("Display", openByDefault))
        {
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

            static bool vsync = true;
            if(ImGui::Checkbox("Vsync", &vsync))
            {
                if(vsync)
                {
                    refs.m_renderer->recreateSwapChainResources(vk::PresentModeKHR::eFifo);
                }
                else
                {
                    refs.m_renderer->recreateSwapChainResources(vk::PresentModeKHR::eMailbox);
                }
            }
        }

        ImGui::End();
    }
}

} // namespace engine
