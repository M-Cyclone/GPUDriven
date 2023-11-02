#include "vk/context.h"

#include <array>
#include <iostream>
#include <string>

#include "vk/descriptorsets.h"
#include "vk/error_vk.h"
#include "vk/pipeline.h"
#include "vk/render_pass.h"

#include "utils/load_shader.h"

Context::Context(const ContextCreateInfo& context_info)
    : instance_layers{ context_info.instance_layers }
    , instance_extensions{ context_info.instance_extensions }
    , device_extensions{ context_info.device_extensions }
{
    createInstance();
    surface = context_info.makeSurfaceFunc(instance);

    pickGPU();
    queryQueueFamilyIndices();
    createDevice();

    querySwapchainInfo(context_info.swapchain_width, context_info.swapchain_height);
    createSwapchain();
    createSwapchainImageView();

    createSwapchainRenderPass();
    createSwapchainFramebuffers();

    createSwapchainPipeline();

    createCmdPoolAndAllocateBuffer();
    createSyncObjects();
}

Context::~Context()
{
    vkDeviceWaitIdle(device);

    destroySyncObjects();
    destroyCmdPoolAndDeallocateBuffer();

    destroySwapchainPipeline();
    destroySwapchainFramebuffers();
    destroySwapchainRenderPass();

    destroySwapchainImageView();
    destroySwapchain();

    destroyDevice();

    vkDestroySurfaceKHR(instance, surface, nullptr);
    destroyInstance();
}

void Context::createInstance()
{
    VkApplicationInfo app_info{};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext              = nullptr;
    app_info.pApplicationName   = "gpu_driven";
    app_info.applicationVersion = 1;
    app_info.pEngineName        = "gpu_driven";
    app_info.engineVersion      = 1;
    app_info.apiVersion         = VK_API_VERSION_1_3;


    // clang-format off
    instance_layers = [&layers = this->instance_layers]()
    {
        std::vector<const char*> final_layers;

        uint32_t count;
        NVVK_CHECK(vkEnumerateInstanceLayerProperties(&count, nullptr));
        std::vector<VkLayerProperties> available_layers(count);
        NVVK_CHECK(vkEnumerateInstanceLayerProperties(&count, available_layers.data()));

        for (const char* required_layer : layers)
        {
            for (const VkLayerProperties& prop : available_layers)
            {
                if (std::strcmp(required_layer, prop.layerName) == 0)
                {
                    final_layers.push_back(required_layer);
                    break;
                }
            }
        }
        return final_layers;
    }();
    // clang-format on

#ifndef NDEBUG
    {
        std::cout << "[Enabled Layers]\n";
        for (const char* layer : instance_layers)
        {
            std::cout << layer << "\n";
        }
        std::cout << std::endl;

        std::cout << "[Enabled Extensions]\n";
        for (const char* ext : instance_extensions)
        {
            std::cout << ext << "\n";
        }
        std::cout << std::endl;
    }
#endif  // !NDEBUG


    VkInstanceCreateInfo info{};
    info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pNext                   = nullptr;
    info.flags                   = 0;
    info.pApplicationInfo        = &app_info;
    info.enabledLayerCount       = static_cast<uint32_t>(instance_layers.size());
    info.ppEnabledLayerNames     = instance_layers.data();
    info.enabledExtensionCount   = static_cast<uint32_t>(instance_extensions.size());
    info.ppEnabledExtensionNames = instance_extensions.data();

    NVVK_CHECK(vkCreateInstance(&info, nullptr, &instance));
}

void Context::destroyInstance()
{
    vkDestroyInstance(instance, nullptr);
}

void Context::pickGPU()
{
    uint32_t count;
    NVVK_CHECK(vkEnumeratePhysicalDevices(instance, &count, nullptr));
    std::vector<VkPhysicalDevice> gpus(count);
    NVVK_CHECK(vkEnumeratePhysicalDevices(instance, &count, gpus.data()));
    active_gpu = gpus[0];
}

void Context::queryQueueFamilyIndices()
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(active_gpu, &count, nullptr);
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(active_gpu, &count, props.data());

    for (uint32_t i = 0; i < count; ++i)
    {
        const VkQueueFamilyProperties& prop = props[i];

        if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queue_family_indices.graphics = i;
        }

        VkBool32 supported = VK_FALSE;
        NVVK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(active_gpu, i, surface, &supported));
        if (supported == VK_TRUE)
        {
            queue_family_indices.present = i;
        }

        if (queue_family_indices.isCompleted())
        {
            break;
        }
    }
}

void Context::createDevice()
{
    float priorities[] = { 1.0f };

    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    {
        std::set<uint32_t> queue_indices = { *queue_family_indices.graphics, *queue_family_indices.present };
        for (uint32_t queue_idx : queue_indices)
        {
            VkDeviceQueueCreateInfo info{};
            info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.pNext            = nullptr;
            info.flags            = 0;
            info.queueFamilyIndex = queue_idx;
            info.queueCount       = 1;
            info.pQueuePriorities = priorities;

            queue_infos.push_back(info);
        }
    }

    VkDeviceCreateInfo info{};
    info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.pNext                   = nullptr;
    info.flags                   = 0;
    info.queueCreateInfoCount    = static_cast<uint32_t>(queue_infos.size());
    info.pQueueCreateInfos       = queue_infos.data();
    info.enabledLayerCount       = 0;
    info.ppEnabledLayerNames     = nullptr;
    info.enabledExtensionCount   = static_cast<uint32_t>(device_extensions.size());
    info.ppEnabledExtensionNames = device_extensions.data();
    info.pEnabledFeatures        = nullptr;
    NVVK_CHECK(vkCreateDevice(active_gpu, &info, nullptr, &device));

    vkGetDeviceQueue(device, *queue_family_indices.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, *queue_family_indices.present, 0, &present_queue);
}

void Context::destroyDevice()
{
    vkDestroyDevice(device, nullptr);
}

void Context::querySwapchainInfo(uint32_t w, uint32_t h)
{
    {
        uint32_t count;
        NVVK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(active_gpu, surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> formats(count);
        NVVK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(active_gpu, surface, &count, formats.data()));

        swapchain_info.format = formats[0];
        for (const VkSurfaceFormatKHR& format : formats)
        {
            if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                swapchain_info.format = format;
                break;
            }
        }
    }


    {
        VkSurfaceCapabilitiesKHR capas;
        NVVK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(active_gpu, surface, &capas));

        swapchain_info.img_count         = std::clamp<uint32_t>(2, capas.minImageCount, capas.maxImageCount);
        swapchain_info.img_extent.width  = std::clamp(w, capas.minImageExtent.width, capas.maxImageExtent.width);
        swapchain_info.img_extent.height = std::clamp(w, capas.minImageExtent.height, capas.maxImageExtent.height);
        swapchain_info.transform         = capas.currentTransform;
    }


    {
        uint32_t count;
        NVVK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(active_gpu, surface, &count, nullptr));
        std::vector<VkPresentModeKHR> presents(count);
        NVVK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(active_gpu, surface, &count, presents.data()));

        swapchain_info.present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& present : presents)
        {
            if (present == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                swapchain_info.present_mode = present;
                break;
            }
        }
    }
}

void Context::createSwapchain()
{
    std::set    queues = { *queue_family_indices.graphics, *queue_family_indices.present };
    std::vector unique_queues(queues.begin(), queues.end());

    VkSwapchainCreateInfoKHR info{};
    info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.pNext                 = nullptr;
    info.flags                 = 0;
    info.surface               = surface;
    info.minImageCount         = swapchain_info.img_count;
    info.imageFormat           = swapchain_info.format.format;
    info.imageColorSpace       = swapchain_info.format.colorSpace;
    info.imageExtent           = swapchain_info.img_extent;
    info.imageArrayLayers      = 1;
    info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode      = unique_queues.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = static_cast<uint32_t>(unique_queues.size());
    info.pQueueFamilyIndices   = unique_queues.data();
    info.preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode           = swapchain_info.present_mode;
    info.clipped               = VK_TRUE;
    info.oldSwapchain          = VK_NULL_HANDLE;

    NVVK_CHECK(vkCreateSwapchainKHR(device, &info, nullptr, &swapchain));


    uint32_t count;
    NVVK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr));
    std::vector<VkImage> images(count);
    NVVK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &count, images.data()));
    for (VkImage img : images)
    {
        swapchain_imgs.push_back(img);
    }
}

void Context::destroySwapchain()
{
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Context::createSwapchainImageView()
{
    const size_t image_count = swapchain_imgs.size();
    swapchain_img_views.resize(image_count);

    for (size_t i = 0; i < image_count; ++i)
    {
        VkComponentMapping mapping{
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        };

        VkImageSubresourceRange range{};
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        VkImageViewCreateInfo info{};
        info.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext            = nullptr;
        info.flags            = 0;
        info.image            = swapchain_imgs[i];
        info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        info.format           = swapchain_info.format.format;
        info.subresourceRange = range;
        info.components       = mapping;

        NVVK_CHECK(vkCreateImageView(device, &info, nullptr, &swapchain_img_views[i]));
    }
}

void Context::destroySwapchainImageView()
{
    for (auto& view : swapchain_img_views)
    {
        vkDestroyImageView(device, view, nullptr);
    }
}

void Context::createSwapchainRenderPass()
{
    swapchain_render_pass = nvvk::createRenderPass(device,
                                                   { swapchain_info.format.format },
                                                   VK_FORMAT_UNDEFINED,
                                                   1,
                                                   true,
                                                   false,
                                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void Context::destroySwapchainRenderPass()
{
    vkDestroyRenderPass(device, swapchain_render_pass, nullptr);
}

void Context::createSwapchainFramebuffers()
{
    const size_t count = swapchain_imgs.size();
    swapchain_framebuffers.resize(count);

    for (size_t i = 0; i < count; ++i)
    {
        VkFramebufferCreateInfo info{};
        info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.pNext           = nullptr;
        info.flags           = 0;
        info.renderPass      = swapchain_render_pass;
        info.attachmentCount = 1;
        info.pAttachments    = &swapchain_img_views[i];
        info.width           = swapchain_info.img_extent.width;
        info.height          = swapchain_info.img_extent.height;
        info.layers          = 1;

        NVVK_CHECK(vkCreateFramebuffer(device, &info, nullptr, &swapchain_framebuffers[i]));
    }
}

void Context::destroySwapchainFramebuffers()
{
    for (VkFramebuffer fb : swapchain_framebuffers)
    {
        vkDestroyFramebuffer(device, fb, nullptr);
    }
}

void Context::createSwapchainPipeline()
{
    swapchain_dset = std::make_unique<nvvk::DescriptorSetContainer>(device);
    swapchain_dset->initLayout();
    swapchain_dset->initPool(1);
    swapchain_dset->initPipeLayout();

    nvvk::GraphicsPipelineState pstate{};
    pstate.rasterizationState.cullMode = VK_CULL_MODE_NONE;

    nvvk::GraphicsPipelineGenerator pgen(device, swapchain_dset->getPipeLayout(), swapchain_render_pass, pstate);
    pgen.addShader(loadShaderCode("test.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT, "main");
    pgen.addShader(loadShaderCode("test.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT, "main");

    swapchain_pipeline = pgen.createPipeline();

    pgen.clearShaders();
}

void Context::destroySwapchainPipeline()
{
    swapchain_dset.reset();
    vkDestroyPipeline(device, swapchain_pipeline, nullptr);
}

void Context::createCmdPoolAndAllocateBuffer()
{
    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext            = nullptr;
    info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = *queue_family_indices.graphics;

    NVVK_CHECK(vkCreateCommandPool(device, &info, nullptr, &cmd_pool));


    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext              = 0;
    alloc_info.commandPool        = cmd_pool;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = swapchain_imgs.size();

    cmds.resize(swapchain_imgs.size());
    NVVK_CHECK(vkAllocateCommandBuffers(device, &alloc_info, cmds.data()));
}

void Context::destroyCmdPoolAndDeallocateBuffer()
{
    vkFreeCommandBuffers(device, cmd_pool, static_cast<uint32_t>(cmds.size()), cmds.data());
    vkDestroyCommandPool(device, cmd_pool, nullptr);
}

void Context::createSyncObjects()
{
    const uint32_t count = static_cast<uint32_t>(swapchain_imgs.size());
    img_draw_finished_semaphores.resize(count);
    img_available_semaphores.resize(count);
    cmd_available_fences.resize(count);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < count; ++i)
    {
        NVVK_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &img_draw_finished_semaphores[i]));
        NVVK_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &img_available_semaphores[i]));
        NVVK_CHECK(vkCreateFence(device, &fence_info, nullptr, &cmd_available_fences[i]));
    }
}

void Context::destroySyncObjects()
{
    for (VkSemaphore s : img_draw_finished_semaphores)
    {
        vkDestroySemaphore(device, s, nullptr);
    }
    for (VkSemaphore s : img_available_semaphores)
    {
        vkDestroySemaphore(device, s, nullptr);
    }
    for (VkFence f : cmd_available_fences)
    {
        vkDestroyFence(device, f, nullptr);
    }
}

void Context::renderToSwapchain()
{
    VkSemaphore img_draw_finished_semaphore = img_draw_finished_semaphores[curr_frame_idx];
    VkSemaphore img_available_semaphore     = img_available_semaphores[curr_frame_idx];
    VkFence     cmd_available_fence         = cmd_available_fences[curr_frame_idx];

    NVVK_CHECK(vkWaitForFences(device, 1, &cmd_available_fence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
    NVVK_CHECK(vkResetFences(device, 1, &cmd_available_fence));

    uint32_t img_idx;
    NVVK_CHECK(vkAcquireNextImageKHR(device,
                                     swapchain,
                                     std::numeric_limits<uint64_t>::max(),
                                     img_available_semaphore,
                                     VK_NULL_HANDLE,
                                     &img_idx));


    VkCommandBuffer cmd = cmds[curr_frame_idx];
    NVVK_CHECK(vkResetCommandBuffer(cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext            = nullptr;
    begin_info.flags            = 0;
    begin_info.pInheritanceInfo = nullptr;
    NVVK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));
    {
        VkRect2D area{
            .offset{ 0, 0 },
            .extent{ swapchain_info.img_extent }
        };
        VkClearValue clear_value{ .color{ .float32{ 0.1f, 0.1f, 0.1f, 1.0f } } };
        VkViewport viewport(0.0f, 0.0f, swapchain_info.img_extent.width, swapchain_info.img_extent.height, 0.0f, 1.0f);

        VkRenderPassBeginInfo render_pass_begin{};
        render_pass_begin.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin.pNext           = nullptr;
        render_pass_begin.renderPass      = swapchain_render_pass;
        render_pass_begin.framebuffer     = swapchain_framebuffers[img_idx];
        render_pass_begin.renderArea      = area;
        render_pass_begin.clearValueCount = 1;
        render_pass_begin.pClearValues    = &clear_value;
        vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, swapchain_pipeline);

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &area);

            vkCmdDraw(cmd, 3, 1, 0, 0);
        }
        vkCmdEndRenderPass(cmd);
    }
    NVVK_CHECK(vkEndCommandBuffer(cmd));


    VkPipelineStageFlags submit_wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submit_info{};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = nullptr;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &img_available_semaphore;
    submit_info.pWaitDstStageMask    = submit_wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &img_draw_finished_semaphore;
    NVVK_CHECK(vkQueueSubmit(graphics_queue, 1, &submit_info, cmd_available_fence));

    VkPresentInfoKHR present_info{};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext              = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &img_draw_finished_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain;
    present_info.pImageIndices      = &img_idx;
    present_info.pResults           = nullptr;
    NVVK_CHECK(vkQueuePresentKHR(present_queue, &present_info));


    curr_frame_idx = (curr_frame_idx + 1) % swapchain_imgs.size();
}
