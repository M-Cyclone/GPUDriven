#include "core/window.h"
#include <cassert>
#include <sstream>

#include <GLFW/glfw3.h>

// ********************************* GlfwWindow Helper *********************************

Window::GLFWHelper Window::GLFWHelper::s_glfw_helper;

Window::GLFWHelper::GLFWHelper() noexcept
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfwSetErrorCallback(
        [](int error_code, const char* description) { throw Window::Exception(-1, "GLFW Internal Code", error_code, description); });
}

Window::GLFWHelper::~GLFWHelper() noexcept
{
    glfwTerminate();
}

inline Window::GLFWHelper& Window::GLFWHelper::get()
{
    return s_glfw_helper;
}

// ********************************* Window Exception *********************************

Window::Exception::Exception(int line, const char* file, int error_code, const char* description) noexcept
    : EngineDefaultException(line, file)
    , m_error_code(error_code)
    , m_desc(description)
{}

const char* Window::Exception::what() const noexcept
{
    std::ostringstream oss;
    oss << getType() << "\n"
        << "[Error Code] " << m_error_code << "\n"
        << "[Description] " << m_desc << "\n"
        << getOriginString();
    m_what_buffer = oss.str();
    return m_what_buffer.c_str();
}

const char* Window::Exception::getType() const noexcept
{
    return "Window Exception";
}

// ************************************ Window **************************************

Window::Window(class App* app, uint32_t width, uint32_t height, const char* title) noexcept
    : m_window{ glfwCreateWindow((int)width, (int)height, title, nullptr, nullptr), glfwDestroyWindow }
    , m_title(title)
    , m_width(width)
    , m_height(height)
{
    glfwSetWindowUserPointer(m_window.get(), app);
}
