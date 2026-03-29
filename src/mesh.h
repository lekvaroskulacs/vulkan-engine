#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "command_buffer.h"

namespace engine
{

class Mesh
{
public:
    std::vector<Vertex> GetVertices() const
    {
        return m_vertices;
    }

    std::vector<uint32_t> GetIndices() const
    {
        return m_indices;
    }

    VkBuffer GetVertexBuffer() const
    {
        return m_vertexBuffer;
    }

    VkBuffer GetIndexBuffer() const
    {
        return m_indexBuffer;
    }

    explicit Mesh(std::shared_ptr<Device> device,
                  std::shared_ptr<CommandBuffer> commandBuffer,
                  std::string modelPath)
        : m_device{device}
        , m_commandBuffer{commandBuffer}
    {
        loadModel(modelPath);
        createVertexBuffer();
        createIndexBuffer();
    }

    ~Mesh()
    {
        vkDestroyBuffer(m_device->GetDevice(), m_indexBuffer, nullptr);
        vkFreeMemory(m_device->GetDevice(), m_indexBufferMemory, nullptr);

        vkFreeMemory(m_device->GetDevice(), m_vertexBufferMemory, nullptr);
        vkDestroyBuffer(m_device->GetDevice(), m_vertexBuffer, nullptr);
    }

private:
    void loadModel(std::string modelPath)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        std::string warn;

        if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str()))
        {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for(const auto& shape : shapes)
        {
            for(const auto& index : shape.mesh.indices)
            {
                Vertex vertex{};
                vertex.m_pos = {attrib.vertices[3 * index.vertex_index + 0],
                                attrib.vertices[3 * index.vertex_index + 1],
                                attrib.vertices[3 * index.vertex_index + 2]};

                vertex.m_texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                                     1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

                vertex.m_color = {1.0f, 1.0f, 1.0f};

                if(uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                    m_vertices.push_back(vertex);
                }

                m_indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = m_commandBuffer->beginSingleTimeCommands();
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        m_commandBuffer->endSingleTimeCommands(commandBuffer);
    }

    void createVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;
        m_device->createBuffer(bufferSize,
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent,
                               stagingBuffer,
                               stagingBufferMemory);

        void* data;
        vkMapMemory(m_device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_device->GetDevice(), stagingBufferMemory);

        m_device->createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            m_vertexBuffer,
            m_vertexBufferMemory);
        copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

        vkDestroyBuffer(m_device->GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_device->GetDevice(), stagingBufferMemory, nullptr);
    }

    void createIndexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;
        m_device->createBuffer(bufferSize,
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent,
                               stagingBuffer,
                               stagingBufferMemory);

        void* data;
        vkMapMemory(m_device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_indices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_device->GetDevice(), stagingBufferMemory);

        m_device->createBuffer(bufferSize,
                               vk::BufferUsageFlagBits::eTransferDst |
                                   vk::BufferUsageFlagBits::eIndexBuffer,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               m_indexBuffer,
                               m_indexBufferMemory);

        copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

        vkDestroyBuffer(m_device->GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_device->GetDevice(), stagingBufferMemory, nullptr);
    }

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