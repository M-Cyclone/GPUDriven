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

#include "core/window.h"
#include "core/keyboard.h"
#include "core/mouse.h"

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
    //void onResize(uint32_t width, uint32_t height);

private:
    void init();
    void update(float delta_time, float total_time);
    void render();
    void exit();

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
    Window m_window;

    Keyboard m_kbd;
    Mouse    m_mouse;

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
    uint8_t m_is_paused  : 1 = false;
};
