#pragma once
#include <memory>
#include <vector>

struct SDL_Window;

class Context;
class DescriptorSetContainer;

class App
{
private:
    inline static constexpr const char* k_window_title  = "GPU Driven";
    inline static constexpr uint32_t    k_window_width  = 1280;
    inline static constexpr uint32_t    k_window_height = 720;

public:
    App();
    ~App();

    void run();

private:
    void init();
    void update();
    void render();
    void exit();

private:
    void onResize(uint32_t width, uint32_t height);

private:
    std::unique_ptr<SDL_Window, void (*)(SDL_Window*)> m_window;

    std::unique_ptr<Context> m_ctx;

    bool m_is_running = true;
};
