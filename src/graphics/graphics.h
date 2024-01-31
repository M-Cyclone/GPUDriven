#pragma once
#include <numeric>
#include <vector>

#include <vulkan/vulkan.h>

#include "graphics/vertex.h"

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
    ~Graphics() noexcept;

    void waitIdle();

    void beginFrame();
    void endFrame();

    void drawTestData(float total_time);

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
    uint32_t                     m_curr_frame_index = 0;

    uint32_t        m_curr_sc_img_index               = 0;
    VkSemaphore     m_curr_sc_render_finish_semaphore = VK_NULL_HANDLE;
    VkSemaphore     m_curr_sc_img_available_semaphore = VK_NULL_HANDLE;
    VkFence         m_curr_cmd_available_fence        = VK_NULL_HANDLE;
    VkCommandBuffer m_curr_cmd                        = VK_NULL_HANDLE;

private:
    void createRenderPass();
    void destroyRenderPass();

    void createFramebuffers();
    void destroyFramebuffers();

    void createResources();
    void destroyResources();

    void createDescriptorSet();
    void destroyDescriptorSet();

    void createPipeline();
    void destroyPipeline();

private:
    VkRenderPass m_render_pass = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> m_framebuffers;

    vertex::Layout              m_vertex_layout;
    VkBuffer                    m_vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory              m_vertex_memory = VK_NULL_HANDLE;
    VkBuffer                    m_index_buffer  = VK_NULL_HANDLE;
    VkDeviceMemory              m_index_memory  = VK_NULL_HANDLE;
    std::vector<VkBuffer>       m_uniform_buffers;
    std::vector<VkDeviceMemory> m_uniform_memorys;
    std::vector<void*>          m_uniform_memptrs;

    VkDescriptorPool             m_descriptor_pool       = VK_NULL_HANDLE;
    VkDescriptorSetLayout        m_descriptor_set_layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_descriptor_sets;

    VkPipelineLayout m_pipeline_layout   = VK_NULL_HANDLE;
    VkPipeline       m_graphics_pipeline = VK_NULL_HANDLE;
};
