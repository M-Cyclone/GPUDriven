#pragma once
#include <memory>
#include <string>

#include "utils/exception.h"

class Window
{
    friend class App;

private:
    class SDLHelper
    {
    private:
        SDLHelper() noexcept;
        SDLHelper(const SDLHelper&)            = delete;
        SDLHelper& operator=(const SDLHelper&) = delete;
        ~SDLHelper() noexcept;

    public:
        static SDLHelper& Get();

    private:
        static SDLHelper s_SDL_helper;
    };

public:
    Window(class App* app, uint32_t width, uint32_t height, const char* title) noexcept;
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    struct SDL_Window* GetNativeWindow() const noexcept { return m_window.get(); }

    float       GetAspectRatio() const noexcept { return (float)m_width / (float)m_height; }
    const char* GetTitle() const noexcept { return m_title.c_str(); }
    uint32_t    GetWidth() const noexcept { return m_width; }
    uint32_t    GetHeight() const noexcept { return m_height; }

private:
    std::unique_ptr<struct SDL_Window, void (*)(SDL_Window*)> m_window;

    std::string m_title;
    uint32_t    m_width;
    uint32_t    m_height;
};
