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
        static SDLHelper& get();

    private:
        static SDLHelper s_SDL_helper;
    };

public:
    Window(class App* app, uint32_t width, uint32_t height, const char* title) noexcept;
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    struct SDL_Window* getNativeWindow() const noexcept { return m_window.get(); }

    float       getAspectRatio() const noexcept { return (float)m_width / (float)m_height; }
    const char* getTitle() const noexcept { return m_title.c_str(); }
    uint32_t    getWidth() const noexcept { return m_width; }
    uint32_t    getHeight() const noexcept { return m_height; }

private:
    std::unique_ptr<struct SDL_Window, void (*)(SDL_Window*)> m_window;

    std::string m_title;
    uint32_t    m_width;
    uint32_t    m_height;
};
