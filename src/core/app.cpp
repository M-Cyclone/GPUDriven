#include "core/app.h"
#include <array>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <thread>

#include <SDL.h>

bool g_init_app = false;

static constexpr const char* k_window_title  = "GPU Driven";
static constexpr uint32_t    k_window_width  = 1280;
static constexpr uint32_t    k_window_height = 720;

App::App()
    : m_window(this, k_window_width, k_window_height, k_window_title)
    , m_gfx(m_window)
{
    assert(!g_init_app);

    g_init_app = true;
}

void App::Run()
{
    auto start = std::chrono::high_resolution_clock::now();
    auto prev  = start;

    while (m_bIsRunning)
    {
        {
            SDL_Event e{};
            while (SDL_PollEvent(&e))
            {
                switch (e.type)
                {
                case SDL_KEYDOWN:
                {
                    m_kbd.OnKeyPressed(e.key);
                    break;
                }
                case SDL_KEYUP:
                {
                    m_kbd.OnKeyReleased(e.key);
                    break;
                }
                case SDL_MOUSEMOTION:
                {
                    // SDL_MouseMotionEvent;
                    break;
                }
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                {
                    // SDL_MouseButtonEvent;
                    break;
                }
                case SDL_MOUSEWHEEL:
                {
                    // SDL_MouseWheelEvent;
                    break;
                }
                default: break;
                }
            }
        }

        if (m_bIsPaused)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        auto curr = std::chrono::high_resolution_clock::now();

        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(curr - prev).count();
        float totalTime = std::chrono::duration<float, std::chrono::seconds::period>(curr - start).count();

        Update(deltaTime, totalTime);

        prev = curr;
    }

    m_gfx.WaitIdle();
}

void App::Update(float deltaTime, float totalTime)
{
    m_gfx.BeginFrame();
    m_gfx.DrawTestData(totalTime);
    m_gfx.EndFrame();

    if (m_kbd.IsKeyPressed(SDL_SCANCODE_ESCAPE))
    {
        m_bIsRunning = false;
    }
}
