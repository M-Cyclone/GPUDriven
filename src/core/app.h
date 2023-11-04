#pragma once
#include <memory>
#include <stdint.h>
#include <vector>

#include <vulkan/vulkan.h>

#include "vk/device.h"
#include "vk/swapchain.h"
#include "vk/descriptorsets.h"

struct GLFWwindow;

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
    void onResize(uint32_t width, uint32_t height);

private:
    void init();
    void exit();

    void update();
    void render();

private:
    void createGraphicsPipeline();
    void destroyGraphicsPipeline();

    void createFramebuffer();
    void destroyFramebuffer();

    void createCmdPoolAndAllocateBuffer();
    void destroyCmdPoolAndDeallocateBuffer();

    void createSyncObjects();
    void destroySyncObjects();

    void createVertexBuffer();
    void destroyVertexBuffer();

private:
    std::unique_ptr<GLFWwindow, void (*)(GLFWwindow*)> m_window;

    Device    m_device;
    Swapchain m_swapchain;

    VkRenderPass               m_render_pass;
    std::vector<VkFramebuffer> m_framebuffers;

    std::unique_ptr<nvvk::DescriptorSetContainer> m_dset;
    VkPipeline                                    m_pipeline;

    VkCommandPool                m_cmd_pool;
    std::vector<VkCommandBuffer> m_cmds;
    std::vector<VkSemaphore>     m_img_draw_finished_semaphores;
    std::vector<VkSemaphore>     m_img_available_semaphores;
    std::vector<VkFence>         m_cmd_available_fences;

    uint32_t m_curr_frame_idx = 0;

    VkBuffer m_vertex_buffer;
    VkDeviceMemory m_vertex_buffer_memory;

    uint8_t m_is_running : 1 = true;
};
