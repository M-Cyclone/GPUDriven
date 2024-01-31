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

void App::run()
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
                case SDL_QUIT:
                {
                    m_bIsRunning = false;
                    break;
                }
                case SDL_KEYDOWN:
                {
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

        update(deltaTime, totalTime);

        prev = curr;
    }

    m_gfx.waitIdle();
}

void App::update(float deltaTime, float totalTime)
{
    m_gfx.beginFrame();
    m_gfx.drawTestData(totalTime);
    m_gfx.endFrame();
}
