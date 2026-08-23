#pragma once

#include <glm/glm.hpp>


namespace engine::utils
{

    struct AABB
    {
        alignas(16) glm::vec4 m_min;
        alignas(16) glm::vec4 m_max;
    };

}