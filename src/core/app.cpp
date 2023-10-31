#include "core/app.h"
#include <stdexcept>

#include <SDL_vulkan.h>
#include <SDL.h>

#include <vulkan/vulkan.h>

#include "vk/context.h"

bool g_init_app = false;

App::App()
    : m_window{ nullptr, SDL_DestroyWindow }
{
    assert(!g_init_app);

    SDL_Init(SDL_INIT_EVERYTHING);

    {
        m_window.reset(SDL_CreateWindow(k_window_title,
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        static_cast<int>(k_window_width),
                                        static_cast<int>(k_window_height),
                                        SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN));

        if (!m_window)
        {
            SDL_Quit();
            throw std::runtime_error("Failed to create window.");
        }
    }

    ContextCreateInfo ci{};
    {
        ci.instance_layers.push_back("VK_LAYER_KHRONOS_validation");

        unsigned int instance_extension_count{};
        SDL_Vulkan_GetInstanceExtensions(m_window.get(), &instance_extension_count, nullptr);
        ci.instance_extensions.resize(instance_extension_count);
        SDL_Vulkan_GetInstanceExtensions(m_window.get(), &instance_extension_count, ci.instance_extensions.data());


        ci.device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        ci.makeSurfaceFunc = [wnd = m_window.get()](VkInstance instance) {
            VkSurfaceKHR surface = VK_NULL_HANDLE;
            if (SDL_Vulkan_CreateSurface(wnd, instance, &surface) != SDL_TRUE)
            {
                throw std::runtime_error("Failed to create vulkan surface.");
            }
            return surface;
        };

        ci.swapchain_width  = k_window_width;
        ci.swapchain_height = k_window_height;
    }
    m_ctx = std::make_unique<Context>(ci);

    g_init_app = true;
}

App::~App()
{
    SDL_Quit();
}

void App::run()
{
    init();

    SDL_Event e{};
    while (m_is_running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                m_is_running = false;
            }
        }

        update();
        render();
    }

    exit();
}

void App::init()
{}

void App::update()
{}

void App::render()
{
    m_ctx->renderToSwapchain();
}

void App::exit()
{
    m_ctx.reset();
}

void App::onResize(uint32_t width, uint32_t height)
{}
