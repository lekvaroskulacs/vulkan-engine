#include "renderer.h"

namespace engine
{

struct UniformUpdateParams
{
    Uniform& m_uniform;
    int currentImage;
    float m_dt;
};

///
/// @brief A Game object consists of a mesh, uniforms, textures and a render pipeline
class GameObject
{
public:
    GameObject(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, std::string modelPath)
        : m_device{device}
        , m_commandBuffer{commandBuffer}
    {
        m_mesh = std::make_unique<Mesh>(device, commandBuffer, modelPath);
    }

    GameObject(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, std::unique_ptr<Mesh>&& mesh)
        : m_device{device}
        , m_commandBuffer{commandBuffer}
        , m_mesh{std::move(mesh)}
    { }

    //TODO: could also take renderpassstage as parameter, and create pipeline for each pass
    template <class UniformType>
    void addUniform(uint8_t binding, vk::ShaderStageFlags shaderStage, std::function<void(Uniform&, int)> updateFuncion)
    {
        auto uniform = std::make_unique<UniformType>(m_device);
        m_pipelineResources[binding] = {.m_stage = shaderStage, .m_resource = uniform.get()};
        m_uniforms.push_back(std::move(uniform));
        m_updateFunctions.push_back(updateFuncion);
    }

    void addTexture(uint8_t binding, vk::ShaderStageFlags shaderStage, TextureParams&& params)
    {
        auto texture = std::make_unique<Texture>(m_device, m_commandBuffer, std::move(params));
        m_pipelineResources[binding] = {.m_stage = shaderStage, .m_resource = texture.get()};
        m_textures.push_back(std::move(texture));
    }

    // FOR DEBUGGING ONLY! Careful with passing raw ptr
    void addTexture(uint8_t binding, vk::ShaderStageFlags shaderStage, Texture* tex)
    {
        m_pipelineResources[binding] = {.m_stage = shaderStage, .m_resource = tex};
    }

    void addLight(glm::vec3* position)
    {
        m_light_position = position;
    }

    void finalizeGameObject(std::shared_ptr<SwapChain> swapChain, const ShaderCodePaths& shaders, bool useShadows = false)
    {
        auto shadowIdx = m_pipelineResources.size();
        if(useShadows)
        {
            m_pipelineResources[shadowIdx] = {.m_stage = vk::ShaderStageFlagBits::eFragment,
                                              .m_resource = swapChain->GetRenderTarget(RenderPassStage::ShadowMap)};
        }

        CreatePipelineParams createParams{};
        createParams.m_shaderPaths = shaders;
        createParams.m_resources = m_pipelineResources;
        m_pipeline = std::make_unique<Pipeline>(m_device, swapChain, createParams);

        if(useShadows)
        {
            createParams.m_resources.erase(shadowIdx);
            createParams.m_shaderPaths = {.m_vertexShaderPath = "shaders/shadow_omni.vert",
                                          .m_fragmentShaderPath = "shaders/shadow_omni.frag"};
            createParams.m_isShadowStage = true;
            m_shadowPipeline = std::make_unique<Pipeline>(m_device, swapChain, createParams);
        }
    }

    DrawFrameParams getDrawFrameParams()
    {
        std::vector<engine::DrawFrameParams::PerRenderPassParams::UniformParam> uniformParamList;
        for(int i = 0; i < m_uniforms.size(); ++i)
        {
            engine::DrawFrameParams::PerRenderPassParams::UniformParam uniformParam{.m_uniform = *m_uniforms[i],
                                                                                    .m_operation = m_updateFunctions[i]};
            uniformParamList.push_back(std::move(uniformParam));
        }

        if(m_shadowPipeline)
        {
            engine::DrawFrameParams::PerRenderPassParams passParam{.m_uniforms = uniformParamList, .m_pipeline = *m_pipeline};
            engine::DrawFrameParams::PerRenderPassParams shadowpassParam{.m_uniforms = uniformParamList,
                                                                         .m_pipeline = *m_shadowPipeline};

            std::unordered_map<RenderPassStage, engine::DrawFrameParams::PerRenderPassParams> passInfos = {
                {RenderPassStage::General, passParam}, {RenderPassStage::ShadowMap, shadowpassParam}};
            DrawFrameParams drawParams{.m_renderPassInfo = passInfos, .m_mesh = *m_mesh, .m_light_position = *m_light_position};

            return drawParams;
        }
        else
        {
            engine::DrawFrameParams::PerRenderPassParams passParam{.m_uniforms = uniformParamList, .m_pipeline = *m_pipeline};

            std::unordered_map<RenderPassStage, engine::DrawFrameParams::PerRenderPassParams> passInfos = {
                {RenderPassStage::General, passParam}};
            DrawFrameParams drawParams{.m_renderPassInfo = passInfos, .m_mesh = *m_mesh};

            return drawParams;
        }
    }

    Pipeline& GetPipeline()
    {
        return *m_pipeline;
    }

    Pipeline& GetShadowPipeline()
    {
        return *m_shadowPipeline;
    }

private:
    glm::vec3* m_light_position;

    std::vector<std::unique_ptr<Uniform>> m_uniforms;
    std::vector<std::unique_ptr<Texture>> m_textures;
    std::unique_ptr<Mesh> m_mesh;
    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<Pipeline> m_shadowPipeline;
    std::unordered_map<uint8_t, engine::PipelineResource> m_pipelineResources;
    std::vector<std::function<void(engine::Uniform&, int)>> m_updateFunctions;

    std::shared_ptr<Device> m_device;
    std::shared_ptr<CommandBuffer> m_commandBuffer;
}; // namespace engine

} // namespace engine