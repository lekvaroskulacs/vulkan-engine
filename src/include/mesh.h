#pragma once

#include "command_buffer.h"
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

    explicit Mesh(std::shared_ptr<Device> device,
                  std::shared_ptr<CommandBuffer> commandBuffer,
                  std::string modelPath);
    ~Mesh();

private:
    void loadModel(std::string modelPath);
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
    void createVertexBuffer();
    void createIndexBuffer();

    std::shared_ptr<Device> m_device;
    std::shared_ptr<engine::CommandBuffer> m_commandBuffer;

    vk::Buffer m_vertexBuffer;
    vk::DeviceMemory m_vertexBufferMemory;
    vk::Buffer m_indexBuffer;
    vk::DeviceMemory m_indexBufferMemory;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

} // namespace engine