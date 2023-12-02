#pragma once
#include <numeric>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "utils/exception.h"

class Window;

class Graphics
{
public:
    static constexpr uint32_t k_invalid_queue_index = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t k_max_in_flight_count = 2;

public:
    class VkException : public EngineDefaultException
    {
    public:
        VkException(int line, const char* file, VkResult result) noexcept;

        const char* what() const noexcept override;
        const char* getType() const noexcept override;

    private:
        VkResult m_result;
    };

public:
    explicit Graphics(Window& window);
    Graphics(const Graphics&)            = delete;
    Graphics& operator=(const Graphics&) = delete;

    void waitIdle();

    void beginFrame();
    void endFrame();

    void drawTestData();

private:
    Window& m_window;

    vk::raii::Context m_context;

    vk::raii::Instance m_instance = nullptr;
#ifdef USE_VULKAN_VALIDATION_LAYER
    vk::raii::DebugUtilsMessengerEXT m_debug_msgr = nullptr;
#endif  // USE_VULKAN_VALIDATION_LAYER

    vk::raii::SurfaceKHR m_surface = nullptr;

    vk::raii::PhysicalDevice m_active_gpu                  = nullptr;
    uint32_t                 m_queue_family_index_graphics = k_invalid_queue_index;
    uint32_t                 m_queue_family_index_present  = k_invalid_queue_index;
    vk::raii::Device         m_device                      = nullptr;

    vk::raii::Queue m_queue_graphics = nullptr;
    vk::raii::Queue m_queue_present  = nullptr;

    vk::SurfaceFormatKHR         m_swapchain_surface_format;
    uint32_t                     m_swapchain_image_count = 0;
    vk::Extent2D                 m_swapchain_image_extent;
    vk::SurfaceTransformFlagsKHR m_swapchain_transform;
    vk::PresentModeKHR           m_swapchain_present_mode;

    vk::raii::SwapchainKHR           m_swapchain = nullptr;
    std::vector<VkImage>             m_swapchain_images;       // Swapchain image is created by swapchain.
    std::vector<vk::raii::ImageView> m_swapchain_image_views;  // Swapchain image view is created by Graphics.

    vk::raii::CommandPool                m_swapchain_image_present_cmd_pool = nullptr;
    std::vector<vk::raii::CommandBuffer> m_swapchain_image_present_cmds;
    std::vector<vk::raii::Semaphore>     m_swapchain_render_finished_semaphores;
    std::vector<vk::raii::Semaphore>     m_swapchain_image_available_semaphores;
    std::vector<vk::raii::Fence>         m_cmd_available_fences;
    uint32_t                             m_curr_frame_index = 0;

    uint32_t        m_curr_sc_img_index               = 0;
    VkSemaphore     m_curr_sc_render_finish_semaphore = VK_NULL_HANDLE;
    VkSemaphore     m_curr_sc_img_available_semaphore = VK_NULL_HANDLE;
    VkFence         m_curr_cmd_available_fence        = VK_NULL_HANDLE;
    VkCommandBuffer m_curr_cmd                        = VK_NULL_HANDLE;
};
