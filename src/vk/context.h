#pragma once
#include <cassert>
#include <functional>
#include <optional>
#include <set>
#include <vector>

#include "vk/descriptorsets.h"

struct ContextCreateInfo
{
    std::vector<const char*> instance_layers;
    std::vector<const char*> instance_extensions;
    std::vector<const char*> device_extensions;

    std::function<VkSurfaceKHR(VkInstance)> makeSurfaceFunc;

    uint32_t swapchain_width;
    uint32_t swapchain_height;
};

class Context final
{
public:
    explicit Context(const ContextCreateInfo& context_info);
    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&)                 = delete;
    void operator=(Context&&)          = delete;
    ~Context();

private:
    void createInstance();
    void destroyInstance();

    void pickGPU();
    void queryQueueFamilyIndices();
    void createDevice();
    void destroyDevice();

    void querySwapchainInfo(uint32_t w, uint32_t h);

    void createSwapchain();
    void destroySwapchain();

    void createSwapchainImageView();
    void destroySwapchainImageView();

    void createSwapchainRenderPass();
    void destroySwapchainRenderPass();

    void createSwapchainFramebuffers();
    void destroySwapchainFramebuffers();

    void createSwapchainPipeline();
    void destroySwapchainPipeline();

    void createCmdPoolAndAllocateBuffer();
    void destroyCmdPoolAndDeallocateBuffer();

    void createSyncObjects();
    void destroySyncObjects();

public:
    void renderToSwapchain();

public:
    VkInstance               instance;
    std::vector<const char*> instance_layers;
    std::vector<const char*> instance_extensions;
    std::vector<const char*> device_extensions;

    VkSurfaceKHR surface;


    struct QueueFamilyIndices final
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;

        [[nodiscard]]
        bool isCompleted() const
        {
            return graphics.has_value() && present.has_value();
        }
    };
    VkPhysicalDevice   active_gpu;
    VkDevice           device;
    QueueFamilyIndices queue_family_indices;
    VkQueue            graphics_queue;
    VkQueue            present_queue;


    struct SwapchainInfo
    {
        VkExtent2D                 img_extent;
        uint32_t                   img_count;
        VkSurfaceFormatKHR         format;
        VkSurfaceTransformFlagsKHR transform;
        VkPresentModeKHR           present_mode;
    } swapchain_info;

    VkSwapchainKHR           swapchain;
    std::vector<VkImage>     swapchain_imgs;
    std::vector<VkImageView> swapchain_img_views;

    VkRenderPass                                  swapchain_render_pass;
    VkPipeline                                    swapchain_pipeline;
    std::unique_ptr<nvvk::DescriptorSetContainer> swapchain_dset;

    std::vector<VkFramebuffer> swapchain_framebuffers;

    VkCommandPool                cmd_pool;
    std::vector<VkCommandBuffer> cmds;
    std::vector<VkSemaphore>     img_draw_finished_semaphores;
    std::vector<VkSemaphore>     img_available_semaphores;
    std::vector<VkFence>         cmd_available_fences;

    uint32_t curr_frame_idx = 0;
};
