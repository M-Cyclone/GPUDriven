#include "graphics/graphics.h"
#include <set>
#include <sstream>
#include <string>

#include <GLFW/glfw3.h>

#include "core/window.h"

#include "utils/load_shader.h"
#include "utils/log.h"
#include "utils/scienum.h"

#include "vk/descriptorsets.h"
#include "vk/pipeline.h"
#include "vk/vertex.h"

#define VK_EXCEPT(call)                                                                                                                    \
    do                                                                                                                                     \
    {                                                                                                                                      \
        if (VkResult res = (call); res != VK_SUCCESS)                                                                                      \
        {                                                                                                                                  \
            throw ::Graphics::VkException(__LINE__, __FILE__, res);                                                                        \
        }                                                                                                                                  \
    } while (false)

Graphics::VkException::VkException(int line, const char* file, VkResult result) noexcept
    : EngineDefaultException(line, file)
    , m_result(result)
{}

const char* Graphics::VkException::what() const noexcept
{
    std::ostringstream oss;
    oss << getType() << "\n"
        << "[Error Code] " << scienum::get_enum_name(m_result) << "\n"
        << getOriginString();
    m_what_buffer = oss.str();
    return m_what_buffer.c_str();
}

const char* Graphics::VkException::getType() const noexcept
{
    return "Vulkan Exception";
}

#ifdef USE_VULKAN_VALIDATION_LAYER
namespace
{
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                    VkDebugUtilsMessageTypeFlagsEXT             type,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                    void*                                       user_data)
{
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LogError("[Validation layer]\n{}.", data->pMessage);
        return VK_FALSE;
    }
    else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LogWarn("[Validation layer]\n{}.", data->pMessage);
        return VK_TRUE;
    }
    else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        LogInfo("[Validation layer]\n{}.", data->pMessage);
        return VK_TRUE;
    }
    else  // if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        LogTrace("[Validation layer]\n{}.", data->pMessage);
        return VK_TRUE;
    }
}
}  // namespace
#endif  // USE_VULKAN_VALIDATION_LAYER

Graphics::Graphics(Window& window)
    : m_window(window)
{
    {
        std::vector<const char*> instance_layers = {
#if USE_VULKAN_VALIDATION_LAYER
            "VK_LAYER_KHRONOS_validation"
#endif
        };
        {
            std::vector<const char*> not_found_layers;

            auto available_instance_layers = m_context.enumerateInstanceLayerProperties();
            for (const char* const layer : instance_layers)
            {
                bool found = false;
                for (const auto& prop : available_instance_layers)
                {
                    if (std::strcmp(layer, prop.layerName) == 0)
                    {
                        found = true;
                    }
                }
                if (!found)
                {
                    not_found_layers.push_back(layer);
                }
            }

            if (!not_found_layers.empty())
            {
                std::ostringstream oss;
                oss << "Failed to load the following layers:\n";
                for (const char* const layer : not_found_layers)
                {
                    oss << layer << "\n";
                }
                std::string str = oss.str();
                LogError(str.c_str());
            }
        }
        std::vector<const char*> instance_extensions;
        {
            uint32_t     glfwExtensionCount = 0;
            const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            std::vector  extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef USE_VULKAN_VALIDATION_LAYER
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif  // USE_VULKAN_VALIDATION_LAYER

            instance_extensions = std::move(extensions);
        }

#ifdef USE_VULKAN_VALIDATION_LAYER
        VkDebugUtilsMessengerCreateInfoEXT debug_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_info.pfnUserCallback = debugCallback;
        debug_info.pUserData       = nullptr;
#endif  // USE_VULKAN_VALIDATION_LAYER

        VkApplicationInfo app_info  = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.pNext              = nullptr;
        app_info.pApplicationName   = m_window.getTitle();
        app_info.applicationVersion = 1;
        app_info.pEngineName        = m_window.getTitle();
        app_info.engineVersion      = 1;
        app_info.apiVersion         = VK_API_VERSION_1_3;

        VkInstanceCreateInfo instance_info    = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instance_info.pNext                   = nullptr;
        instance_info.flags                   = 0;
        instance_info.pApplicationInfo        = &app_info;
        instance_info.enabledLayerCount       = static_cast<uint32_t>(instance_layers.size());
        instance_info.ppEnabledLayerNames     = instance_layers.empty() ? nullptr : instance_layers.data();
        instance_info.enabledExtensionCount   = static_cast<uint32_t>(instance_extensions.size());
        instance_info.ppEnabledExtensionNames = instance_extensions.empty() ? nullptr : instance_extensions.data();

#ifdef USE_VULKAN_VALIDATION_LAYER
        instance_info.pNext = &debug_info;
#endif  // USE_VULKAN_VALIDATION_LAYER

        m_instance = vk::raii::Instance(m_context, instance_info);

#ifdef USE_VULKAN_VALIDATION_LAYER
        m_debug_msgr = vk::raii::DebugUtilsMessengerEXT(m_instance, debug_info);
#endif  // USE_VULKAN_VALIDATION_LAYER
    }

    {
        VkSurfaceKHR surface;
        glfwCreateWindowSurface(*m_instance, m_window.getNativeWindow(), nullptr, &surface);
        m_surface = vk::raii::SurfaceKHR(m_instance, surface);
    }

    {
        auto gpus    = m_instance.enumeratePhysicalDevices();
        m_active_gpu = gpus[0];

        auto     queue_props      = m_active_gpu.getQueueFamilyProperties();
        uint32_t queue_family_idx = 0;
        for (const auto& prop : queue_props)
        {
            if (prop.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                m_queue_family_index_graphics = queue_family_idx;
            }

            if (m_active_gpu.getSurfaceSupportKHR(queue_family_idx, *m_surface))
            {
                m_queue_family_index_present = queue_family_idx;
            }

            if (m_queue_family_index_graphics != k_invalid_queue_index && m_queue_family_index_present != k_invalid_queue_index)
            {
                break;
            }

            queue_family_idx++;
        }


        float priorities[] = { 1.0f };

        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        {
            std::set<uint32_t> queue_indices = { m_queue_family_index_graphics, m_queue_family_index_present };
            for (uint32_t queue_idx : queue_indices)
            {
                VkDeviceQueueCreateInfo info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
                info.pNext                   = nullptr;
                info.flags                   = 0;
                info.queueFamilyIndex        = queue_idx;
                info.queueCount              = 1;
                info.pQueuePriorities        = priorities;

                queue_infos.push_back(info);
            }
        }

        std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        {
            std::vector<const char*> not_found_extensions;

            auto available_device_extensions = m_active_gpu.enumerateDeviceExtensionProperties();
            for (const char* const extension : device_extensions)
            {
                bool found = false;
                for (const auto& prop : available_device_extensions)
                {
                    if (std::strcmp(extension, prop.extensionName) == 0)
                    {
                        found = true;
                    }
                }
                if (!found)
                {
                    not_found_extensions.push_back(extension);
                }
            }

            if (!not_found_extensions.empty())
            {
                std::ostringstream oss;
                oss << "Failed to load the following layers:\n";
                for (const char* const ext : not_found_extensions)
                {
                    oss << ext << "\n";
                }
                std::string str = oss.str();
                LogError(str.c_str());
            }
        }

        VkPhysicalDeviceFeatures device_features = { .samplerAnisotropy = VK_TRUE };

        VkDeviceCreateInfo device_info      = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        device_info.pNext                   = nullptr;
        device_info.flags                   = 0;
        device_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_infos.size());
        device_info.pQueueCreateInfos       = queue_infos.data();
        device_info.enabledLayerCount       = 0;        // deprecated
        device_info.ppEnabledLayerNames     = nullptr;  // deprecated
        device_info.enabledExtensionCount   = static_cast<uint32_t>(device_extensions.size());
        device_info.ppEnabledExtensionNames = device_extensions.empty() ? nullptr : device_extensions.data();
        device_info.pEnabledFeatures        = &device_features;

        m_device = vk::raii::Device(m_active_gpu, device_info);


        m_queue_graphics = m_device.getQueue(m_queue_family_index_graphics, 0);
        m_queue_present  = m_device.getQueue(m_queue_family_index_present, 0);
    }

    {
        {
            auto formats               = m_active_gpu.getSurfaceFormatsKHR(*m_surface);
            m_swapchain_surface_format = formats[0];
            for (const vk::SurfaceFormatKHR& format : formats)
            {
                if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                {
                    m_swapchain_surface_format = format;
                    break;
                }
            }
        }

        {
            auto capas = m_active_gpu.getSurfaceCapabilitiesKHR(*m_surface);

            m_swapchain_image_count         = std::clamp<uint32_t>(2, capas.minImageCount, capas.maxImageCount);
            m_swapchain_image_extent.width  = std::clamp(m_window.getWidth(), capas.minImageExtent.width, capas.maxImageExtent.width);
            m_swapchain_image_extent.height = std::clamp(m_window.getHeight(), capas.minImageExtent.height, capas.maxImageExtent.height);
            m_swapchain_transform           = capas.currentTransform;
        }

        {
            auto present_modes       = m_active_gpu.getSurfacePresentModesKHR(*m_surface);
            m_swapchain_present_mode = vk::PresentModeKHR::eFifo;
            for (const auto& mode : present_modes)
            {
                if (mode == vk::PresentModeKHR::eMailbox)
                {
                    m_swapchain_present_mode = mode;
                    break;
                }
            }
        }

        std::set<uint32_t>    unique_queues = { m_queue_family_index_graphics, m_queue_family_index_present };
        std::vector<uint32_t> queues(unique_queues.begin(), unique_queues.end());

        VkSwapchainCreateInfoKHR swapchain_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchain_info.pNext                    = nullptr;
        swapchain_info.flags                    = 0;
        swapchain_info.surface                  = *m_surface;
        swapchain_info.minImageCount            = m_swapchain_image_count;
        swapchain_info.imageFormat              = (VkFormat)m_swapchain_surface_format.format;
        swapchain_info.imageColorSpace          = (VkColorSpaceKHR)m_swapchain_surface_format.colorSpace;
        swapchain_info.imageExtent              = m_swapchain_image_extent;
        swapchain_info.imageArrayLayers         = 1;
        swapchain_info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_info.imageSharingMode         = queues.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount    = static_cast<uint32_t>(queues.size());
        swapchain_info.pQueueFamilyIndices      = queues.data();
        swapchain_info.preTransform             = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchain_info.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_info.presentMode              = (VkPresentModeKHR)m_swapchain_present_mode;
        swapchain_info.clipped                  = VK_TRUE;
        swapchain_info.oldSwapchain             = VK_NULL_HANDLE;

        m_swapchain = vk::raii::SwapchainKHR(m_device, swapchain_info);

        {
            {
                auto images             = m_swapchain.getImages();
                m_swapchain_image_count = (uint32_t)images.size();
                for (vk::Image image : images)
                {
                    m_swapchain_images.push_back(image);
                }
            }

            for (uint32_t i = 0; i < m_swapchain_image_count; ++i)
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

                VkImageViewCreateInfo image_view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
                image_view_info.pNext                 = nullptr;
                image_view_info.flags                 = 0;
                image_view_info.image                 = m_swapchain_images[i];
                image_view_info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
                image_view_info.format                = (VkFormat)m_swapchain_surface_format.format;
                image_view_info.subresourceRange      = range;
                image_view_info.components            = mapping;

                m_swapchain_image_views.emplace_back(m_device, image_view_info);
            }
        }
    }

    {
        VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        pool_info.pNext                   = nullptr;
        pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex        = m_queue_family_index_graphics;

        m_swapchain_image_present_cmd_pool = vk::raii::CommandPool(m_device, pool_info);


        VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        alloc_info.pNext                       = 0;
        alloc_info.commandPool                 = *m_swapchain_image_present_cmd_pool;
        alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount          = m_swapchain_image_count;

        m_swapchain_image_present_cmds = m_device.allocateCommandBuffers(alloc_info);
    }

    {
        m_swapchain_render_finished_semaphores.reserve(k_max_in_flight_count);
        m_swapchain_image_available_semaphores.reserve(k_max_in_flight_count);
        m_cmd_available_fences.reserve(k_max_in_flight_count);

        VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        semaphore_info.pNext                 = nullptr;
        semaphore_info.flags                 = 0;

        VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fence_info.pNext             = nullptr;
        fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < k_max_in_flight_count; ++i)
        {
            m_swapchain_render_finished_semaphores.emplace_back(m_device, semaphore_info);
            m_swapchain_image_available_semaphores.emplace_back(m_device, semaphore_info);
            m_cmd_available_fences.emplace_back(m_device, fence_info);
        }
    }
}

void Graphics::waitIdle()
{
    m_device.waitIdle();
}

void Graphics::beginFrame()
{
    m_curr_sc_render_finish_semaphore = *m_swapchain_render_finished_semaphores[m_curr_frame_index];
    m_curr_sc_img_available_semaphore = *m_swapchain_image_available_semaphores[m_curr_frame_index];
    m_curr_cmd_available_fence        = *m_cmd_available_fences[m_curr_frame_index];

    m_curr_cmd = *m_swapchain_image_present_cmds[m_curr_frame_index];

    VK_EXCEPT(vkWaitForFences(*m_device, 1, &m_curr_cmd_available_fence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
    VK_EXCEPT(vkResetFences(*m_device, 1, &m_curr_cmd_available_fence));

    VK_EXCEPT(vkAcquireNextImageKHR(*m_device,
                                    *m_swapchain,
                                    std::numeric_limits<uint64_t>::max(),
                                    m_curr_sc_img_available_semaphore,
                                    VK_NULL_HANDLE,
                                    &m_curr_sc_img_index));

    VK_EXCEPT(vkResetCommandBuffer(m_curr_cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

void Graphics::endFrame()
{
    // VkSwapchainKHR swapchains[] = { *m_swapchain };

    // VkPresentInfoKHR present_info   = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    // present_info.pNext              = nullptr;
    // present_info.waitSemaphoreCount = 1;
    // present_info.pWaitSemaphores    = &m_curr_sc_render_finish_semaphore;
    // present_info.swapchainCount     = (uint32_t)std::size(swapchains);
    // present_info.pSwapchains        = swapchains;
    // present_info.pImageIndices      = &m_curr_frame_index;
    // present_info.pResults           = nullptr;

    // VK_EXCEPT(vkQueuePresentKHR(*m_queue_graphics, &present_info));
}

void Graphics::drawTestData()
{
    vk::raii::RenderPass render_pass = nullptr;
    {
        std::vector<VkAttachmentDescription> allAttachments;
        std::vector<VkAttachmentReference>   color_attachment_refs;

        VkAttachmentDescription attachment = {};
        attachment.format                  = (VkFormat)m_swapchain_surface_format.format;
        attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference attachment_ref = {};
        attachment_ref.attachment            = static_cast<uint32_t>(allAttachments.size());
        attachment_ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        allAttachments.push_back(attachment);
        color_attachment_refs.push_back(attachment_ref);


        std::vector<VkSubpassDescription> subpasses;
        std::vector<VkSubpassDependency>  subpassDependencies;

        for (uint32_t i = 0; i < 1; i++)
        {
            VkSubpassDescription subpass    = {};
            subpass.flags                   = 0;
            subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.inputAttachmentCount    = 0;
            subpass.pInputAttachments       = nullptr;
            subpass.colorAttachmentCount    = static_cast<uint32_t>(color_attachment_refs.size());
            subpass.pColorAttachments       = color_attachment_refs.data();
            subpass.pResolveAttachments     = nullptr;
            subpass.pDepthStencilAttachment = nullptr;
            subpass.preserveAttachmentCount = 0;
            subpass.pPreserveAttachments    = nullptr;

            VkSubpassDependency dependency  = {};
            dependency.srcSubpass           = i == 0 ? (VK_SUBPASS_EXTERNAL) : (i - 1);
            dependency.dstSubpass           = i;
            dependency.srcStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask        = 0;
            dependency.dstAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependency.srcStageMask        |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask        |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask       |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            subpasses.push_back(subpass);
            subpassDependencies.push_back(dependency);
        }

        VkRenderPassCreateInfo render_pass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        render_pass_info.attachmentCount        = static_cast<uint32_t>(allAttachments.size());
        render_pass_info.pAttachments           = allAttachments.data();
        render_pass_info.subpassCount           = static_cast<uint32_t>(subpasses.size());
        render_pass_info.pSubpasses             = subpasses.data();
        render_pass_info.dependencyCount        = static_cast<uint32_t>(subpassDependencies.size());
        render_pass_info.pDependencies          = subpassDependencies.data();

        render_pass = vk::raii::RenderPass(m_device, render_pass_info);
    }

    vk::raii::Framebuffer framebuffer = nullptr;
    {
        std::array<VkImageView, 1> attachments = { *m_swapchain_image_views[m_curr_sc_img_index] };

        VkFramebufferCreateInfo framebuffer_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.pNext                   = nullptr;
        framebuffer_info.flags                   = 0;
        framebuffer_info.renderPass              = *render_pass;
        framebuffer_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
        framebuffer_info.pAttachments            = attachments.data();
        framebuffer_info.width                   = m_swapchain_image_extent.width;
        framebuffer_info.height                  = m_swapchain_image_extent.height;
        framebuffer_info.layers                  = 1;

        framebuffer = vk::raii::Framebuffer(m_device, framebuffer_info);
    }

    nvvk::DescriptorSetContainer dset(*m_device);
    vk::raii::Pipeline           graphics_pipeline = nullptr;
    {
        dset.initLayout();
        dset.initPool(k_max_in_flight_count);
        dset.initPipeLayout();


        const uint32_t binding = 0;


        nvvk::GraphicsPipelineState pstate{};
        pstate.rasterizationState.cullMode = VK_CULL_MODE_NONE;

        nvvk::GraphicsPipelineGenerator pgen(*m_device, dset.getPipeLayout(), *render_pass, pstate);
        pgen.addShader(loadShaderCode("test.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT, "main");
        pgen.addShader(loadShaderCode("test.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT, "main");

        VkPipeline pipeline = pgen.createPipeline();
        graphics_pipeline   = vk::raii::Pipeline(m_device, pipeline);

        pgen.clearShaders();
    }


    VkCommandBuffer cmd = m_curr_cmd;

    VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.pNext                    = nullptr;
    begin_info.flags                    = 0;
    begin_info.pInheritanceInfo         = nullptr;
    VK_EXCEPT(vkBeginCommandBuffer(cmd, &begin_info));
    {
        VkClearValue clear_color = { .color{ .float32{ 0.1f, 0.1f, 0.1f, 1.0f } } };

        VkExtent2D extent = m_swapchain_image_extent;
        VkRect2D   area{
              .offset{ 0, 0 },
              .extent{ extent }
        };
        VkViewport viewport(0.0f, 0.0f, extent.width, extent.height, 0.0f, 1.0f);

        VkRenderPassBeginInfo render_pass_begin{};
        render_pass_begin.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin.pNext           = nullptr;
        render_pass_begin.renderPass      = *render_pass;
        render_pass_begin.framebuffer     = *framebuffer;
        render_pass_begin.renderArea      = area;
        render_pass_begin.clearValueCount = 1;
        render_pass_begin.pClearValues    = &clear_color;
        vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindDescriptorSets(cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    dset.getPipeLayout(),
                                    0,
                                    1,
                                    dset.getSets(m_curr_frame_index),
                                    0,
                                    nullptr);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphics_pipeline);

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &area);

            vkCmdDraw(cmd, 3, 1, 0, 0);
        }
        vkCmdEndRenderPass(cmd);
    }
    VK_EXCEPT(vkEndCommandBuffer(cmd));


    VkPipelineStageFlags submit_wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submit_info{};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = nullptr;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &m_curr_sc_img_available_semaphore;
    submit_info.pWaitDstStageMask    = submit_wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &m_curr_sc_render_finish_semaphore;
    VK_EXCEPT(vkQueueSubmit(*m_queue_graphics, 1, &submit_info, m_curr_cmd_available_fence));


    VkSwapchainKHR swapchains[] = { *m_swapchain };

    VkPresentInfoKHR present_info   = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.pNext              = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &m_curr_sc_render_finish_semaphore;
    present_info.swapchainCount     = (uint32_t)std::size(swapchains);
    present_info.pSwapchains        = swapchains;
    present_info.pImageIndices      = &m_curr_sc_img_index;
    present_info.pResults           = nullptr;

    VK_EXCEPT(vkQueuePresentKHR(*m_queue_graphics, &present_info));


    m_curr_frame_index = (m_curr_frame_index + 1) % k_max_in_flight_count;

    waitIdle();
}
