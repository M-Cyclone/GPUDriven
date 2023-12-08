#pragma once
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "core/window.h"
#include "core/keyboard.h"
#include "core/mouse.h"

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

    void onChar(unsigned int codepoint);
    void onCursorEnter(int entered);
    void onCursorPos(double xpos, double ypos);
    void onDrop(int path_count, const char* paths[]);
    void onFramebufferSize(int width, int height);
    void onKey(int key, int scancode, int action, int mods);
    void onMouseButton(int button, int action, int mods);
    void onScroll(double xoffset, double yoffset);
    void onWindowClose();
    void onWindowFocus(int focused);

private:
    Window m_window;

    Keyboard m_kbd;
    Mouse    m_mouse;

    Graphics m_gfx;

    // std::vector<Buffer> m_uniform_buffers;
    // std::vector<void*>  m_uniform_buffers_mapped;

    // Image     m_texture;
    // VkSampler m_sampler;

    // Image    m_color_buffer;
    // VkFormat m_depth_format;
    // Image    m_depth_buffer;

    uint8_t m_is_running : 1 = true;
    uint8_t m_is_paused  : 1 = false;
};
