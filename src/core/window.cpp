#include "core/window.h"
#include <cassert>
#include <sstream>

#include <SDL.h>

// ********************************* GlfwWindow Helper *********************************

Window::SDLHelper Window::SDLHelper::s_SDL_helper;

Window::SDLHelper::SDLHelper() noexcept
{
    SDL_Init(SDL_INIT_EVERYTHING);
}

Window::SDLHelper::~SDLHelper() noexcept
{
    SDL_Quit();
}

inline Window::SDLHelper& Window::SDLHelper::Get()
{
    return s_SDL_helper;
}

// ************************************ Window **************************************

Window::Window(class App* app, uint32_t width, uint32_t height, const char* title) noexcept
    : m_window{ SDL_CreateWindow(title,
                                 SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED,
                                 (int)width,
                                 (int)height,
                                 SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN),
                SDL_DestroyWindow }
    , m_title(title)
    , m_width(width)
    , m_height(height)
{}
