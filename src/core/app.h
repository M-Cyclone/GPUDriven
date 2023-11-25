#pragma once
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "vk/device.h"
#include "vk/swapchain.h"
#include "vk/descriptorsets.h"

#include "vk/vertex.h"
#include "vk/resource.h"
#include "vk/allocator.h"

struct GLFWwindow;

class App
{
private:
    inline static constexpr const char* k_window_title  = "GPU Driven";
    inline static constexpr uint32_t    k_window_width  = 1280;
    inline static constexpr uint32_t    k_window_height = 720;

    inline static constexpr uint32_t k_max_in_flight_count = 2;

public:
    App();
    ~App();

    void run();

private:
    void onResize(uint32_t width, uint32_t height);

private:
    void init();
    void exit();

    void update(float delta_time, float total_time);
    void render();

private:
    void createCmdPoolAndAllocateBuffer();
    void destroyCmdPoolAndDeallocateBuffer();

    void createVertexBuffer();
    void destroyVertexBuffer();

    void createIndexBuffer();
    void destroyIndexBuffer();

    void createUniformBuffers();
    void destroyUniformBuffers();

    void createAttachmentBuffer();
    void destroyAttachmentBuffer();

    void createTextureImageAndSampler();
    void destroyTextureImageAndSampler();

    void createRenderPass();
    void destroyRenderPass();

    void createGraphicsPipeline();
    void destroyGraphicsPipeline();

    void createFramebuffer();
    void destroyFramebuffer();

    void createSyncObjects();
    void destroySyncObjects();

public:
    VkCommandBuffer createTempCommandBuffer() const;
    void            submitAndWaitTempCommandBuffer(VkCommandBuffer cmd, VkQueue queue) const;
    void            freeTempCommandBuffer(VkCommandBuffer cmd) const;

private:
    std::unique_ptr<GLFWwindow, void (*)(GLFWwindow*)> m_window;

    std::string m_title        = k_window_title;
    uint32_t    m_width        = k_window_width;
    uint32_t    m_height       = k_window_height;
    float       m_aspect_ratio = 1.0f;

    Device    m_device;
    Swapchain m_swapchain;

    VkSampleCountFlagBits m_msaa_samples;

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

    std::unique_ptr<Allocator> m_alloc;

    vertex::Layout m_layout;
    Buffer         m_vertex_buffer;
    Buffer         m_index_buffer;

    std::vector<Buffer> m_uniform_buffers;
    std::vector<void*>  m_uniform_buffers_mapped;

    Image     m_texture;
    VkSampler m_sampler;

    Image    m_color_buffer;
    VkFormat m_depth_format;
    Image    m_depth_buffer;

    uint8_t m_is_running : 1 = true;
};
