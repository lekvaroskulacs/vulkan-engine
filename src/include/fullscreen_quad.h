#pragma once

#include "mesh.h"

namespace engine
{

class FullscreenQuadMesh : public Mesh
{
public:
    FullscreenQuadMesh(std::shared_ptr<Device> device, std::shared_ptr<CommandBuffer> commandBuffer)
        : Mesh()
    {
        m_device = device;
        m_commandBuffer = commandBuffer;

        // Define fullscreen quad vertices (NDC coordinates, covering [-1, 1] in X and Y)
        m_vertices = {
            // positions           // colors         // texCoords
            {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
            {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
            {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
            {{-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        };

        m_indices = {0, 2, 1, 2, 0, 3};

        createVertexBuffer();
        createIndexBuffer();
    }
};

} // namespace engine