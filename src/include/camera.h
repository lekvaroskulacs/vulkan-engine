#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace engine
{

class Camera
{
public:
    Camera(GLFWwindow* window)
    {
        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, engine::Camera::mouseCallback);
    }

    glm::vec3 m_cameraPos = glm::vec3(2.0f, 0.0f, 2.0f);
    glm::vec3 m_cameraFront = glm::normalize(glm::vec3(-1.0f, 0.0f, -1.0f));
    glm::vec3 m_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -135.0f; // Facing (-1, -1, 0)
    float pitch = 0.0f;
    float lastX = 400.0f;
    float lastY = 300.0f;
    bool firstMouse = true;
    float sensitivity = 0.1f;

    void processInput(GLFWwindow* window, float dt)
    {
        float cameraSpeed = 0.0005f;
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            m_cameraPos += cameraSpeed * glm::normalize(m_cameraFront - m_cameraPos);
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            m_cameraPos -= cameraSpeed * glm::normalize(m_cameraFront - m_cameraPos);
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            m_cameraPos -= glm::normalize(glm::cross(m_cameraFront - m_cameraPos, m_cameraUp)) * cameraSpeed;
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            m_cameraPos += glm::normalize(glm::cross(m_cameraFront - m_cameraPos, m_cameraUp)) * cameraSpeed;
        if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            m_cameraPos.y -= cameraSpeed;
        if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            m_cameraPos.y += cameraSpeed;
    }

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos)
    {
        Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));

        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cam->firstMouse = true;
            return;
        }

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if(cam->firstMouse)
        {
            cam->lastX = (float)xpos;
            cam->lastY = (float)ypos;
            cam->firstMouse = false;
        }

        float xoffset = (float)xpos - cam->lastX;
        float yoffset = cam->lastY - (float)ypos; // reversed since y-coordinates go from bottom to top
        cam->lastX = (float)xpos;
        cam->lastY = (float)ypos;

        xoffset *= cam->sensitivity;
        yoffset *= cam->sensitivity;

        cam->yaw += xoffset;
        cam->pitch += yoffset;

        // Constrain pitch
        if(cam->pitch > 89.0f)
            cam->pitch = 89.0f;
        if(cam->pitch < -89.0f)
            cam->pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch));
        front.y = sin(glm::radians(cam->pitch));
        front.z = sin(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch));
        cam->m_cameraFront = glm::normalize(front);
    }
};

} // namespace engine