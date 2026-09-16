#pragma once

#include <engine/mesh/mesh.h>

namespace engine
{

class PlaneMesh : public Mesh
{
public:
    // Builds a plane in the XZ plane (normal +Y), centered at the origin, spanning
    // `width` x `length` and subdivided into `widthSegments` x `lengthSegments` quads.
    PlaneMesh(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer, float width = 1.0f,
              float length = 1.0f, uint32_t resolutionX = 1, uint32_t resolutionY = 1)
        : Mesh()
    {
        m_device = device;
        m_commandBuffer = commandBuffer;

        uint32_t vertsPerRow = resolutionX + 1;
        uint32_t vertsPerCol = resolutionY + 1;

        m_vertices.reserve(vertsPerRow * vertsPerCol);

        for (uint32_t z = 0; z < vertsPerCol; ++z)
        {
            for (uint32_t x = 0; x < vertsPerRow; ++x)
            {
                float u = static_cast<float>(x) / resolutionX;
                float v = static_cast<float>(z) / resolutionY;

                float posX = (u - 0.5f) * width;
                float posZ = (v - 0.5f) * length;

                m_vertices.push_back({{posX, 0.0f, posZ}, {0.0f, 1.0f, 0.0f}, {u, v}});
            }
        }

        m_indices.reserve(static_cast<size_t>(resolutionX) * resolutionY * 6);

        for (uint32_t z = 0; z < resolutionY; ++z)
        {
            for (uint32_t x = 0; x < resolutionX; ++x)
            {
                uint32_t topLeft = z * vertsPerRow + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (z + 1) * vertsPerRow + x;
                uint32_t bottomRight = bottomLeft + 1;

                m_indices.push_back(topLeft);
                m_indices.push_back(bottomLeft);
                m_indices.push_back(topRight);

                m_indices.push_back(topRight);
                m_indices.push_back(bottomLeft);
                m_indices.push_back(bottomRight);
            }
        }

        createVertexBuffer();
        createIndexBuffer();
    }
};

} // namespace engine