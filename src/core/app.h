#pragma once
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "core/keyboard.h"
#include "core/mouse.h"
#include "core/window.h"

#include "graphics/graphics.h"

class App
{
private:
    inline static constexpr uint32_t k_max_in_flight_count = 2;

public:
    App();
    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    void run();

private:
    void update(float delta_time, float total_time);

private:
    Window m_window;

    Keyboard m_kbd;
    Keyboard m_kbd;
    Mouse    m_mouse;

    Graphics m_gfx;

    uint8_t m_bIsRunning : 1 = true;
    uint8_t m_bIsPaused  : 1 = false;
};
