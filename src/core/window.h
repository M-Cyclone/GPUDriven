#pragma once
#include <memory>
#include <string>

#include "utils/exception.h"

class Window
{
    friend class App;

private:
    class GLFWHelper
    {
    private:
        GLFWHelper() noexcept;
        GLFWHelper(const GLFWHelper&)            = delete;
        GLFWHelper& operator=(const GLFWHelper&) = delete;
        ~GLFWHelper() noexcept;

    public:
        static GLFWHelper& get();

    private:
        static GLFWHelper s_glfw_helper;
    };

public:
    class Exception final : public EngineDefaultException
    {
    public:
        Exception(int line, const char* file, int error_code, const char* description) noexcept;

        const char* what() const noexcept override;
        const char* getType() const noexcept override;

    private:
        int         m_error_code;
        std::string m_desc;
    };

public:
    Window(class App* app, uint32_t width, uint32_t height, const char* title) noexcept;
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    struct GLFWwindow* getNativeWindow() const noexcept { return m_window.get(); }

    float       getAspectRatio() const noexcept { return (float)m_width / (float)m_height; }
    const char* getTitle() const noexcept { return m_title.c_str(); }
    uint32_t    getWidth() const noexcept { return m_width; }
    uint32_t    getHeight() const noexcept { return m_height; }

private:
    std::unique_ptr<struct GLFWwindow, void (*)(GLFWwindow*)> m_window;

    std::string m_title;
    uint32_t    m_width;
    uint32_t    m_height;
};
