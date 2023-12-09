#pragma once
#include <numeric>
#include <vector>
#include <span>

#include <vulkan/vulkan.h>

#include "utils/exception.h"

class Window;

class Graphics
{
    friend class GraphicsAvailable;

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
    ~Graphics() noexcept;

    void waitIdle();

    void beginFrame();
    void endFrame();

    void drawTestData();

    void drawIndexed(uint32_t count);

    void updateDescriptorSets(std::span<const VkWriteDescriptorSet> writes, std::span<const VkCopyDescriptorSet> copies);

private:
    VkCommandBuffer getCurrSwapchainCmd() noexcept { return m_swapchain_image_present_cmds[m_curr_frame_index]; }

    VkSemaphore getCurrSwapchainRenderFinishSemaphore() noexcept { return m_swapchain_render_finished_semaphores[m_curr_frame_index]; }
    VkSemaphore getCurrSwapchainImgAvailableSemaphore() noexcept { return m_swapchain_image_available_semaphores[m_curr_frame_index]; }
    VkFence     getCurrSwapchainCmdAvailableFence() noexcept { return m_cmd_available_fences[m_curr_frame_index]; }

private:
    Window& m_window;

    VkInstance m_instance = VK_NULL_HANDLE;
#ifdef USE_VULKAN_VALIDATION_LAYER
    PFN_vkCreateDebugUtilsMessengerEXT  m_vkCreateDebugUtilsMessengerEXT  = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT m_vkDestroyDebugUtilsMessengerEXT = nullptr;
    VkDebugUtilsMessengerEXT            m_debug_msgr                      = VK_NULL_HANDLE;
#endif  // USE_VULKAN_VALIDATION_LAYER

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    VkPhysicalDevice m_active_gpu                  = VK_NULL_HANDLE;
    uint32_t         m_queue_family_index_graphics = k_invalid_queue_index;
    uint32_t         m_queue_family_index_present  = k_invalid_queue_index;
    VkDevice         m_device                      = VK_NULL_HANDLE;

    VkQueue m_queue_graphics = VK_NULL_HANDLE;
    VkQueue m_queue_present  = VK_NULL_HANDLE;

    VkSurfaceFormatKHR         m_swapchain_surface_format;
    uint32_t                   m_swapchain_image_count = 0;
    VkExtent2D                 m_swapchain_image_extent;
    VkSurfaceTransformFlagsKHR m_swapchain_transform;
    VkPresentModeKHR           m_swapchain_present_mode;

    VkSwapchainKHR           m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage>     m_swapchain_images;       // Swapchain image is created by swapchain.
    std::vector<VkImageView> m_swapchain_image_views;  // Swapchain image view is created by Graphics.

    VkCommandPool                m_swapchain_image_present_cmd_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_swapchain_image_present_cmds;
    std::vector<VkSemaphore>     m_swapchain_render_finished_semaphores;
    std::vector<VkSemaphore>     m_swapchain_image_available_semaphores;
    std::vector<VkFence>         m_cmd_available_fences;

    uint32_t m_curr_frame_index  = 0;
    uint32_t m_curr_sc_img_index = 0;
};
