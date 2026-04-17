#include "mesh.h"
#include "device.h"
#include "vertex.h"
#include <cstring>
#include <stdexcept>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#pragma GCC system_header
#include <tiny_obj_loader.h>

namespace engine
{

std::vector<Vertex> Mesh::GetVertices() const
{
    return m_vertices;
}

std::vector<uint32_t> Mesh::GetIndices() const
{
    return m_indices;
}

VkBuffer Mesh::GetVertexBuffer() const
{
    return m_vertexBuffer;
}

VkBuffer Mesh::GetIndexBuffer() const
{
    return m_indexBuffer;
}

Mesh::Mesh(std::shared_ptr<Device> device,
           std::shared_ptr<CommandBuffer> commandBuffer,
           std::string modelPath)
    : m_device{device}
    , m_commandBuffer{commandBuffer}
{
    loadModel(modelPath);
    createVertexBuffer();
    createIndexBuffer();
}

Mesh::~Mesh()
{
    m_device->GetDevice().destroyBuffer(m_indexBuffer, nullptr);
    m_device->GetDevice().freeMemory(m_indexBufferMemory, nullptr);
    m_device->GetDevice().freeMemory(m_vertexBufferMemory, nullptr);
    m_device->GetDevice().destroyBuffer(m_vertexBuffer, nullptr);
}

void Mesh::loadModel(std::string modelPath)
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

void Mesh::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
    vk::CommandBuffer commandBuffer = m_commandBuffer->beginSingleTimeCommands();
    vk::BufferCopy copyRegion{
        .size = size,
    };
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    m_commandBuffer->endSingleTimeCommands(commandBuffer);
}

void Mesh::createVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    m_device->createBuffer(bufferSize,
                           vk::BufferUsageFlagBits::eTransferSrc,
                           vk::MemoryPropertyFlagBits::eHostVisible |
                               vk::MemoryPropertyFlagBits::eHostCoherent,
                           stagingBuffer,
                           stagingBufferMemory);

    void* data;
    [[maybe_unused]] auto ignored =
        m_device->GetDevice().mapMemory(stagingBufferMemory, 0, bufferSize, {}, &data);
    memcpy(data, m_vertices.data(), (size_t)bufferSize);
    m_device->GetDevice().unmapMemory(stagingBufferMemory);

    m_device->createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        m_vertexBuffer,
        m_vertexBufferMemory);
    copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    m_device->GetDevice().destroyBuffer(stagingBuffer, nullptr);
    m_device->GetDevice().freeMemory(stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    m_device->createBuffer(bufferSize,
                           vk::BufferUsageFlagBits::eTransferSrc,
                           vk::MemoryPropertyFlagBits::eHostVisible |
                               vk::MemoryPropertyFlagBits::eHostCoherent,
                           stagingBuffer,
                           stagingBufferMemory);

    void* data;
    [[maybe_unused]] auto ignored =
        m_device->GetDevice().mapMemory(stagingBufferMemory, 0, bufferSize, {}, &data);
    memcpy(data, m_indices.data(), (size_t)bufferSize);
    m_device->GetDevice().unmapMemory(stagingBufferMemory);

    m_device->createBuffer(bufferSize,
                           vk::BufferUsageFlagBits::eTransferDst |
                               vk::BufferUsageFlagBits::eIndexBuffer,
                           vk::MemoryPropertyFlagBits::eDeviceLocal,
                           m_indexBuffer,
                           m_indexBufferMemory);

    copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

    m_device->GetDevice().destroyBuffer(stagingBuffer, nullptr);
    m_device->GetDevice().freeMemory(stagingBufferMemory, nullptr);
}

} // namespace engine
