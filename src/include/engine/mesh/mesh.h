#pragma once

#include <engine/command_buffer/command_buffer.h>
#include <engine/vertex/vertex.h>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#pragma GCC system_header
#include <tiny_obj_loader.h>

namespace engine
{

class Mesh
{
public:
    std::vector<Vertex> GetVertices() const;
    std::vector<uint32_t> GetIndices() const;
    VkBuffer GetVertexBuffer() const;
    VkBuffer GetIndexBuffer() const;

    explicit Mesh(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, std::string modelPath);
    explicit Mesh();
    ~Mesh();

protected:
    void loadModel(std::string modelPath);
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
    void createVertexBuffer();
    void createIndexBuffer();

    std::shared_ptr<Device> m_device;
    std::shared_ptr<engine::CommandBuffer> m_commandBuffer;

    vk::Buffer m_vertexBuffer;
    VmaAllocation m_vertexBufferAllocation;
    vk::Buffer m_indexBuffer;
    VmaAllocation m_indexBufferAllocation;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

} // namespace engine