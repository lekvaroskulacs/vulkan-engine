#include "../include/mesh.h"
#include "../include/device.h"
#include "../include/vertex.h"
#include <cstring>
#include <stdexcept>
#include <unordered_map>

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

Mesh::Mesh(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, std::string modelPath)
    : m_device{device}
    , m_commandBuffer{commandBuffer}
{
    loadModel(modelPath);
    createVertexBuffer();
    createIndexBuffer();
}

Mesh::Mesh() { }

Mesh::~Mesh()
{
    m_device->destroyBuffer(m_indexBuffer, m_indexBufferAllocation);
    m_device->destroyBuffer(m_vertexBuffer, m_vertexBufferAllocation);
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
    VmaAllocation stagingBufferAllocation;
    m_device->createBuffer(bufferSize,
                           vk::BufferUsageFlagBits::eTransferSrc,
                           vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                           stagingBuffer,
                           stagingBufferAllocation);

    m_device->copyMemoryToAllocation(m_vertices.data(), stagingBufferAllocation, (size_t)bufferSize);

    m_device->createBuffer(bufferSize,
                           vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                           vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                           m_vertexBuffer,
                           m_vertexBufferAllocation);
    copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    m_device->destroyBuffer(stagingBuffer, stagingBufferAllocation);
}

void Mesh::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

    vk::Buffer stagingBuffer;
    VmaAllocation stagingBufferAllocation;
    m_device->createBuffer(bufferSize,
                           vk::BufferUsageFlagBits::eTransferSrc,
                           vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                           stagingBuffer,
                           stagingBufferAllocation);

    m_device->copyMemoryToAllocation(m_indices.data(), stagingBufferAllocation, (size_t)bufferSize);

    m_device->createBuffer(bufferSize,
                           vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                           vk::MemoryPropertyFlagBits::eDeviceLocal,
                           m_indexBuffer,
                           m_indexBufferAllocation);

    copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

    m_device->destroyBuffer(stagingBuffer, stagingBufferAllocation);
}

} // namespace engine
