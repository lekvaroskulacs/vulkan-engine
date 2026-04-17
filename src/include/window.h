#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace engine
{

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class Window
{
public:
    Window()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Engine", nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
    }

    ~Window()
    {
        glfwDestroyWindow(m_window);
    }

    void SetResizeCallback(GLFWframebuffersizefun callback)
    {
        glfwSetFramebufferSizeCallback(m_window, callback);
    }

    GLFWwindow* Get()
    {
        return m_window;
    }

private:
    GLFWwindow* m_window;
};

} // namespace engine