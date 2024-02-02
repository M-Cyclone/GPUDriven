#include "graphics/graphics.h"
#include <set>
#include <sstream>
#include <string>

#include <SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb/stb_image.h>

#include "shader_header/device.h"
#include "shader_header/vertex_info.h"

#include "core/window.h"

#include "utils/load_shader.h"
#include "utils/log.h"
#include "utils/scienum.h"

#include "graphics/descriptorsets.h"
#include "graphics/pipeline.h"
#include "graphics/vertex.h"

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

std::vector<VkLayerProperties> enumerateInstanceLayerProperties()
{
    uint32_t count = 0;
    VK_EXCEPT(vkEnumerateInstanceLayerProperties(&count, nullptr));
    std::vector<VkLayerProperties> props(count);
    VK_EXCEPT(vkEnumerateInstanceLayerProperties(&count, props.data()));
    return props;
}

std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance)
{
    uint32_t count = 0;
    VK_EXCEPT(vkEnumeratePhysicalDevices(instance, &count, nullptr));
    std::vector<VkPhysicalDevice> gpus(count);
    VK_EXCEPT(vkEnumeratePhysicalDevices(instance, &count, gpus.data()));
    return gpus;
}

std::vector<VkQueueFamilyProperties> getQueueFamilyProperties(VkPhysicalDevice gpu)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, nullptr);
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, props.data());
    return props;
}

bool getSurfaceSupportKHR(VkPhysicalDevice gpu, uint32_t queue_family_index, VkSurfaceKHR surface)
{
    VkBool32 result;
    VK_EXCEPT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queue_family_index, surface, &result));
    return result == VK_TRUE;
}

std::vector<VkExtensionProperties> enumerateDeviceExtensionProperties(VkPhysicalDevice gpu, const char* layer_name = nullptr)
{
    uint32_t count = 0;
    VK_EXCEPT(vkEnumerateDeviceExtensionProperties(gpu, layer_name, &count, nullptr));
    std::vector<VkExtensionProperties> props(count);
    VK_EXCEPT(vkEnumerateDeviceExtensionProperties(gpu, layer_name, &count, props.data()));
    return props;
}

std::vector<VkSurfaceFormatKHR> getSurfaceFormatsKHR(VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
    uint32_t count = 0;
    VK_EXCEPT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, nullptr));
    std::vector<VkSurfaceFormatKHR> formats(count);
    VK_EXCEPT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, formats.data()));
    return formats;
}

VkSurfaceCapabilitiesKHR getSurfaceCapabilitiesKHR(VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
    VkSurfaceCapabilitiesKHR capabilities;
    VK_EXCEPT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capabilities));
    return capabilities;
}

std::vector<VkPresentModeKHR> getSurfacePresentModesKHR(VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
    uint32_t count = 0;
    VK_EXCEPT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count, nullptr));
    std::vector<VkPresentModeKHR> present_modes(count);
    VK_EXCEPT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count, present_modes.data()));
    return present_modes;
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

            auto available_instance_layers = enumerateInstanceLayerProperties();
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
            unsigned int instanceExtensionCount = 0;
            SDL_Vulkan_GetInstanceExtensions(window.GetNativeWindow(), &instanceExtensionCount, nullptr);
            instance_extensions.resize(instanceExtensionCount);
            SDL_Vulkan_GetInstanceExtensions(window.GetNativeWindow(), &instanceExtensionCount, instance_extensions.data());

#ifdef USE_VULKAN_VALIDATION_LAYER
            instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif  // USE_VULKAN_VALIDATION_LAYER

            instance_extensions.erase(std::unique(instance_extensions.begin(), instance_extensions.end()), instance_extensions.end());
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
        app_info.pApplicationName   = m_window.GetTitle();
        app_info.applicationVersion = 1;
        app_info.pEngineName        = m_window.GetTitle();
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

        VK_EXCEPT(vkCreateInstance(&instance_info, nullptr, &m_instance));

#ifdef USE_VULKAN_VALIDATION_LAYER
        {
            m_vkCreateDebugUtilsMessengerEXT =
                (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
            m_vkDestroyDebugUtilsMessengerEXT =
                (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");

            if (!m_vkCreateDebugUtilsMessengerEXT || !m_vkDestroyDebugUtilsMessengerEXT)
            {
                throw VkException(__LINE__, __FILE__, VK_ERROR_EXTENSION_NOT_PRESENT);
            }

            VK_EXCEPT(m_vkCreateDebugUtilsMessengerEXT(m_instance, &debug_info, nullptr, &m_debugMsgr));
        }
#endif  // USE_VULKAN_VALIDATION_LAYER
    }

    {
        assert(SDL_Vulkan_CreateSurface(m_window.GetNativeWindow(), m_instance, &m_surface) == SDL_TRUE);
    }

    {
        auto gpus   = enumeratePhysicalDevices(m_instance);
        m_activeGPU = gpus[0];

        auto     queue_props      = getQueueFamilyProperties(m_activeGPU);
        uint32_t queue_family_idx = 0;
        for (const auto& prop : queue_props)
        {
            if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                m_queueFamilyIndexGraphics = queue_family_idx;
            }

            if (getSurfaceSupportKHR(m_activeGPU, queue_family_idx, m_surface))
            {
                m_queueFamilyIndexPresent = queue_family_idx;
            }

            if (m_queueFamilyIndexGraphics != kInvalidQueueIndex && m_queueFamilyIndexPresent != kInvalidQueueIndex)
            {
                break;
            }

            queue_family_idx++;
        }


        float priorities[] = { 1.0f };

        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        {
            std::set<uint32_t> queue_indices = { m_queueFamilyIndexGraphics, m_queueFamilyIndexPresent };
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

            auto available_device_extensions = enumerateDeviceExtensionProperties(m_activeGPU);
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

        VK_EXCEPT(vkCreateDevice(m_activeGPU, &device_info, nullptr, &m_device));


        vkGetDeviceQueue(m_device, m_queueFamilyIndexGraphics, 0, &m_queueGraphics);
        vkGetDeviceQueue(m_device, m_queueFamilyIndexPresent, 0, &m_queuePresent);
    }

    {
        {
            auto formats             = getSurfaceFormatsKHR(m_activeGPU, m_surface);
            m_swapchainSurfaceFormat = formats[0];
            for (const VkSurfaceFormatKHR& format : formats)
            {
                if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    m_swapchainSurfaceFormat = format;
                    break;
                }
            }
        }

        {
            auto capas = getSurfaceCapabilitiesKHR(m_activeGPU, m_surface);

            m_swapchainImageCount         = std::clamp<uint32_t>(2, capas.minImageCount, capas.maxImageCount);
            m_swapchainImageExtent.width  = std::clamp(m_window.GetWidth(), capas.minImageExtent.width, capas.maxImageExtent.width);
            m_swapchainImageExtent.height = std::clamp(m_window.GetHeight(), capas.minImageExtent.height, capas.maxImageExtent.height);
            m_swapchainTransform          = capas.currentTransform;
        }

        {
            auto present_modes     = getSurfacePresentModesKHR(m_activeGPU, m_surface);
            m_swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
            for (const auto& mode : present_modes)
            {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    m_swapchainPresentMode = mode;
                    break;
                }
            }
        }

        std::set<uint32_t>    unique_queues = { m_queueFamilyIndexGraphics, m_queueFamilyIndexPresent };
        std::vector<uint32_t> queues(unique_queues.begin(), unique_queues.end());

        VkSwapchainCreateInfoKHR swapchain_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchain_info.pNext                    = nullptr;
        swapchain_info.flags                    = 0;
        swapchain_info.surface                  = m_surface;
        swapchain_info.minImageCount            = m_swapchainImageCount;
        swapchain_info.imageFormat              = m_swapchainSurfaceFormat.format;
        swapchain_info.imageColorSpace          = m_swapchainSurfaceFormat.colorSpace;
        swapchain_info.imageExtent              = m_swapchainImageExtent;
        swapchain_info.imageArrayLayers         = 1;
        swapchain_info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_info.imageSharingMode         = queues.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount    = static_cast<uint32_t>(queues.size());
        swapchain_info.pQueueFamilyIndices      = queues.data();
        swapchain_info.preTransform             = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchain_info.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_info.presentMode              = (VkPresentModeKHR)m_swapchainPresentMode;
        swapchain_info.clipped                  = VK_TRUE;
        swapchain_info.oldSwapchain             = VK_NULL_HANDLE;

        VK_EXCEPT(vkCreateSwapchainKHR(m_device, &swapchain_info, nullptr, &m_swapchain));

        {
            VK_EXCEPT(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchainImageCount, nullptr));
            m_swapchainImages.resize(m_swapchainImageCount);
            VK_EXCEPT(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchainImageCount, m_swapchainImages.data()));

            m_swapchainImageViews.resize(m_swapchainImageCount);

            for (uint32_t i = 0; i < m_swapchainImageCount; ++i)
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
                image_view_info.image                 = m_swapchainImages[i];
                image_view_info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
                image_view_info.format                = (VkFormat)m_swapchainSurfaceFormat.format;
                image_view_info.subresourceRange      = range;
                image_view_info.components            = mapping;

                VK_EXCEPT(vkCreateImageView(m_device, &image_view_info, nullptr, &m_swapchainImageViews[i]));
            }
        }
    }

    {
        VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        pool_info.pNext                   = nullptr;
        pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex        = m_queueFamilyIndexGraphics;

        VK_EXCEPT(vkCreateCommandPool(m_device, &pool_info, nullptr, &m_swapchainImagePresentCmdPool));


        VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        alloc_info.pNext                       = 0;
        alloc_info.commandPool                 = m_swapchainImagePresentCmdPool;
        alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount          = m_swapchainImageCount;

        m_swapchainImagePresentCmds.resize(m_swapchainImageCount);
        VK_EXCEPT(vkAllocateCommandBuffers(m_device, &alloc_info, m_swapchainImagePresentCmds.data()));
    }

    {
        m_swapchainRenderFinishedSemaphores.resize(kMaxInFlightCount, VK_NULL_HANDLE);
        m_swapchainImageAvailableSemaphores.resize(kMaxInFlightCount, VK_NULL_HANDLE);
        m_cmdAvailableFences.resize(kMaxInFlightCount, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        semaphore_info.pNext                 = nullptr;
        semaphore_info.flags                 = 0;

        VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fence_info.pNext             = nullptr;
        fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < kMaxInFlightCount; ++i)
        {
            VK_EXCEPT(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_swapchainRenderFinishedSemaphores[i]));
            VK_EXCEPT(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_swapchainImageAvailableSemaphores[i]));
            VK_EXCEPT(vkCreateFence(m_device, &fence_info, nullptr, &m_cmdAvailableFences[i]));
        }
    }

    {
        CreateRenderPass();
        CreateFramebuffers();
        CreateResources();
        CreateDescriptorSet();
        CreatePipeline();
    }
}

Graphics::~Graphics() noexcept
{
    DestroyPipeline();
    DestroyDescriptorSet();
    DestroyResources();
    DestroyFramebuffers();
    DestroyRenderPass();

    for (VkSemaphore s : m_swapchainRenderFinishedSemaphores)
    {
        if (s != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_device, s, nullptr);
        }
    }
    m_swapchainRenderFinishedSemaphores.clear();
    for (VkSemaphore s : m_swapchainImageAvailableSemaphores)
    {
        if (s != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_device, s, nullptr);
        }
    }
    m_swapchainImageAvailableSemaphores.clear();
    for (VkFence f : m_cmdAvailableFences)
    {
        if (f != VK_NULL_HANDLE)
        {
            vkDestroyFence(m_device, f, nullptr);
        }
    }
    m_cmdAvailableFences.clear();

    if (m_swapchainImagePresentCmdPool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(m_device, m_swapchainImagePresentCmdPool, m_swapchainImageCount, m_swapchainImagePresentCmds.data());
        vkDestroyCommandPool(m_device, m_swapchainImagePresentCmdPool, nullptr);
        m_swapchainImagePresentCmds.clear();
        m_swapchainImagePresentCmdPool = VK_NULL_HANDLE;
    }

    if (m_swapchain != VK_NULL_HANDLE)
    {
        for (VkImageView view : m_swapchainImageViews)
        {
            vkDestroyImageView(m_device, view, nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchainImageViews.clear();
        m_swapchain = VK_NULL_HANDLE;
    }

    if (m_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if (m_debugMsgr != VK_NULL_HANDLE)
    {
        m_vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMsgr, nullptr);
        m_debugMsgr = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

void Graphics::WaitIdle()
{
    VK_EXCEPT(vkDeviceWaitIdle(m_device));
}

void Graphics::BeginFrame()
{
    m_currSCRenderFinishSemaphore = m_swapchainRenderFinishedSemaphores[m_currFrameIndex];
    m_currSCImgAvailableSemaphore = m_swapchainImageAvailableSemaphores[m_currFrameIndex];
    m_currCmdAvailableFence       = m_cmdAvailableFences[m_currFrameIndex];

    m_currCmd = m_swapchainImagePresentCmds[m_currFrameIndex];

    VK_EXCEPT(vkWaitForFences(m_device, 1, &m_currCmdAvailableFence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
    VK_EXCEPT(vkResetFences(m_device, 1, &m_currCmdAvailableFence));

    VK_EXCEPT(vkAcquireNextImageKHR(m_device,
                                    m_swapchain,
                                    std::numeric_limits<uint64_t>::max(),
                                    m_currSCImgAvailableSemaphore,
                                    VK_NULL_HANDLE,
                                    &m_currSCImgIndex));

    VK_EXCEPT(vkResetCommandBuffer(m_currCmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

void Graphics::EndFrame()
{
    VkCommandBuffer cmd = m_currCmd;


    VkPipelineStageFlags submit_wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submit_info{};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = nullptr;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &m_currSCImgAvailableSemaphore;
    submit_info.pWaitDstStageMask    = submit_wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &m_currSCRenderFinishSemaphore;
    VK_EXCEPT(vkQueueSubmit(m_queueGraphics, 1, &submit_info, m_currCmdAvailableFence));


    VkSwapchainKHR swapchains[] = { m_swapchain };

    VkPresentInfoKHR present_info   = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.pNext              = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &m_currSCRenderFinishSemaphore;
    present_info.swapchainCount     = (uint32_t)std::size(swapchains);
    present_info.pSwapchains        = swapchains;
    present_info.pImageIndices      = &m_currSCImgIndex;
    present_info.pResults           = nullptr;

    VK_EXCEPT(vkQueuePresentKHR(m_queueGraphics, &present_info));


    m_currFrameIndex = (m_currFrameIndex + 1) % kMaxInFlightCount;

    WaitIdle();
}

void Graphics::DrawTestData(float total_time)
{
    {
        UniformBufferObject ubo{};
        ubo.model       = glm::rotate(glm::mat4(1.0f), (float)total_time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view        = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj        = glm::perspective(glm::radians(45.0f), m_window.GetAspectRatio(), 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        std::memcpy(m_uniformMemPtrs[m_currFrameIndex], &ubo, sizeof(UniformBufferObject));
    }


    VkCommandBuffer cmd = m_currCmd;

    VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.pNext                    = nullptr;
    begin_info.flags                    = 0;
    begin_info.pInheritanceInfo         = nullptr;
    VK_EXCEPT(vkBeginCommandBuffer(cmd, &begin_info));
    {
        VkClearValue clear_color = { .color{ .float32{ 0.1f, 0.1f, 0.1f, 1.0f } } };

        VkExtent2D extent = m_swapchainImageExtent;
        VkRect2D   area{
              .offset{ 0, 0 },
              .extent{ extent }
        };
        VkViewport viewport(0.0f, 0.0f, extent.width, extent.height, 0.0f, 1.0f);

        VkRenderPassBeginInfo render_pass_begin{};
        render_pass_begin.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin.pNext           = nullptr;
        render_pass_begin.renderPass      = m_renderPass;
        render_pass_begin.framebuffer     = m_framebuffers[m_currFrameIndex];
        render_pass_begin.renderArea      = area;
        render_pass_begin.clearValueCount = 1;
        render_pass_begin.pClearValues    = &clear_color;
        vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindDescriptorSets(cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_pipelineLayout,
                                    0,
                                    1,
                                    &m_descriptorSets[m_currFrameIndex],
                                    0,
                                    nullptr);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &area);

            VkBuffer     vertex_buffers[] = { m_vertexBuffer };
            VkDeviceSize offsets[]        = { 0 };

            vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(cmd, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(cmd);
    }
    VK_EXCEPT(vkEndCommandBuffer(cmd));
}

void Graphics::CreateRenderPass()
{
    std::vector<VkAttachmentDescription> allAttachments;
    std::vector<VkAttachmentReference>   color_attachment_refs;

    VkAttachmentDescription attachment = {};
    attachment.format                  = (VkFormat)m_swapchainSurfaceFormat.format;
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

    VK_EXCEPT(vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_renderPass));
}

void Graphics::DestroyRenderPass()
{
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
}

void Graphics::CreateFramebuffers()
{
    for (VkImageView view : m_swapchainImageViews)
    {
        VkFramebufferCreateInfo framebuffer_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.pNext                   = nullptr;
        framebuffer_info.flags                   = 0;
        framebuffer_info.renderPass              = m_renderPass;
        framebuffer_info.attachmentCount         = 1;
        framebuffer_info.pAttachments            = &view;
        framebuffer_info.width                   = m_swapchainImageExtent.width;
        framebuffer_info.height                  = m_swapchainImageExtent.height;
        framebuffer_info.layers                  = 1;

        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VK_EXCEPT(vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &framebuffer));

        m_framebuffers.push_back(framebuffer);
    }
}

void Graphics::DestroyFramebuffers()
{
    for (VkFramebuffer fb : m_framebuffers)
    {
        vkDestroyFramebuffer(m_device, fb, nullptr);
    }
}

void Graphics::CreateResources()
{
    static auto createBuffer =
        [this](VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory) {
        static auto findMemoryType = [this](uint32_t type_filter, VkMemoryPropertyFlags properties) -> uint32_t {
            VkPhysicalDeviceMemoryProperties available_properties;
            vkGetPhysicalDeviceMemoryProperties(m_activeGPU, &available_properties);

            for (uint32_t i = 0; i < available_properties.memoryTypeCount; i++)
            {
                if ((type_filter & (1 << i)) && (available_properties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }

            throw std::runtime_error("failed to find suitable memory type!");
        };

        VkBufferCreateInfo buffer_info    = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        buffer_info.pNext                 = nullptr;
        buffer_info.flags                 = 0;
        buffer_info.size                  = size;
        buffer_info.usage                 = usage;
        buffer_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        buffer_info.queueFamilyIndexCount = 0;
        buffer_info.pQueueFamilyIndices   = nullptr;

        VK_EXCEPT(vkCreateBuffer(m_device, &buffer_info, nullptr, &buffer));

        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(m_device, buffer, &requirements);

        VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocate_info.pNext                = nullptr;
        allocate_info.allocationSize       = requirements.size;
        allocate_info.memoryTypeIndex =
            findMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VK_EXCEPT(vkAllocateMemory(m_device, &allocate_info, nullptr, &memory));

        VK_EXCEPT(vkBindBufferMemory(m_device, buffer, memory, 0));
    };

    static auto copyBuffer = [this](VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool                 = m_swapchainImagePresentCmdPool;
        alloc_info.commandBufferCount          = 1;

        VkCommandBuffer cmd;
        VK_EXCEPT(vkAllocateCommandBuffers(m_device, &alloc_info, &cmd));


        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_EXCEPT(vkBeginCommandBuffer(cmd, &beginInfo));
        {
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size      = size;
            vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);
        }
        VK_EXCEPT(vkEndCommandBuffer(cmd));


        VkSubmitInfo submit_info       = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &cmd;

        VK_EXCEPT(vkQueueSubmit(m_queueGraphics, 1, &submit_info, VK_NULL_HANDLE));
        VK_EXCEPT(vkQueueWaitIdle(m_queueGraphics));


        vkFreeCommandBuffers(m_device, m_swapchainImagePresentCmdPool, 1, &cmd);
    };

    {
        m_vertexLayout.append(vertex::AttributeType::Pos3d);
        m_vertexLayout.append(vertex::AttributeType::Color3);

        vertex::Buffer vb(m_vertexLayout, 4);
        vb[0].attr<vertex::AttributeType::Pos3d>()  = { -0.5f, -0.5f, +0.0f };
        vb[1].attr<vertex::AttributeType::Pos3d>()  = { +0.5f, -0.5f, +0.0f };
        vb[2].attr<vertex::AttributeType::Pos3d>()  = { +0.5f, +0.5f, +0.0f };
        vb[3].attr<vertex::AttributeType::Pos3d>()  = { -0.5f, +0.5f, +0.0f };
        vb[0].attr<vertex::AttributeType::Color3>() = { 1.0f, 0.0f, 0.0f };
        vb[1].attr<vertex::AttributeType::Color3>() = { 0.0f, 1.0f, 0.0f };
        vb[2].attr<vertex::AttributeType::Color3>() = { 0.0f, 0.0f, 1.0f };
        vb[3].attr<vertex::AttributeType::Color3>() = { 1.0f, 1.0f, 0.0f };

        VkDeviceSize size = (VkDeviceSize)vb.sizeOf();


        VkBuffer       staging_buffer;
        VkDeviceMemory staging_memory;
        createBuffer(size,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     staging_buffer,
                     staging_memory);

        void* data;
        VK_EXCEPT(vkMapMemory(m_device, staging_memory, 0, size, 0, &data));
        std::memcpy(data, vb.dataPtr(), size);
        vkUnmapMemory(m_device, staging_memory);


        createBuffer(size,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_vertexBuffer,
                     m_vertexMemory);

        copyBuffer(staging_buffer, m_vertexBuffer, size);


        vkDestroyBuffer(m_device, staging_buffer, nullptr);
        vkFreeMemory(m_device, staging_memory, nullptr);
    }

    {
        std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };
        VkDeviceSize          size    = (VkDeviceSize)(sizeof(uint16_t) * indices.size());


        VkBuffer       staging_buffer;
        VkDeviceMemory staging_memory;
        createBuffer(size,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     staging_buffer,
                     staging_memory);

        void* data;
        VK_EXCEPT(vkMapMemory(m_device, staging_memory, 0, size, 0, &data));
        std::memcpy(data, indices.data(), size);
        vkUnmapMemory(m_device, staging_memory);


        createBuffer(size,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_indexBuffer,
                     m_indexMemory);

        copyBuffer(staging_buffer, m_indexBuffer, size);


        vkDestroyBuffer(m_device, staging_buffer, nullptr);
        vkFreeMemory(m_device, staging_memory, nullptr);
    }

    {
        m_uniformBuffers.resize(kMaxInFlightCount);
        m_uniformMemorys.resize(kMaxInFlightCount);
        m_uniformMemPtrs.resize(kMaxInFlightCount);

        for (size_t i = 0; i < kMaxInFlightCount; ++i)
        {
            VkDeviceSize size = (VkDeviceSize)(sizeof(UniformBufferObject));

            createBuffer(size,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         m_uniformBuffers[i],
                         m_uniformMemorys[i]);


            void* data;
            VK_EXCEPT(vkMapMemory(m_device, m_uniformMemorys[i], 0, size, 0, &data));
            m_uniformMemPtrs[i] = data;
        }
    }
}

void Graphics::DestroyResources()
{
    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexMemory, nullptr);
    vkFreeMemory(m_device, m_indexMemory, nullptr);

    for (size_t i = 0; i < kMaxInFlightCount; ++i)
    {
        vkUnmapMemory(m_device, m_uniformMemorys[i]);

        vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_device, m_uniformMemorys[i], nullptr);
    }
}

void Graphics::CreateDescriptorSet()
{
    VkDescriptorPoolSize pool_size{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = (uint32_t)kMaxInFlightCount };

    VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.pNext                      = nullptr;
    pool_info.flags                      = 0;
    pool_info.maxSets                    = (uint32_t)kMaxInFlightCount;
    pool_info.poolSizeCount              = 1;
    pool_info.pPoolSizes                 = &pool_size;

    VK_EXCEPT(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_descriptorPool));


    VkDescriptorSetLayoutBinding ubo_layout_binding = {};
    ubo_layout_binding.binding                      = BINDING_UBO;
    ubo_layout_binding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount              = 1;
    ubo_layout_binding.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers           = nullptr;

    VkDescriptorSetLayoutCreateInfo set_layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    set_layout_info.pNext                           = nullptr;
    set_layout_info.flags                           = 0;
    set_layout_info.bindingCount                    = 1;
    set_layout_info.pBindings                       = &ubo_layout_binding;

    VK_EXCEPT(vkCreateDescriptorSetLayout(m_device, &set_layout_info, nullptr, &m_descriptorSetLayout));


    std::vector<VkDescriptorSetLayout> layouts(kMaxInFlightCount, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo        descriptor_set_alloc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    descriptor_set_alloc_info.pNext                              = nullptr;
    descriptor_set_alloc_info.descriptorPool                     = m_descriptorPool;
    descriptor_set_alloc_info.descriptorSetCount                 = (uint32_t)kMaxInFlightCount;
    descriptor_set_alloc_info.pSetLayouts                        = layouts.data();

    m_descriptorSets.resize(kMaxInFlightCount);
    VK_EXCEPT(vkAllocateDescriptorSets(m_device, &descriptor_set_alloc_info, m_descriptorSets.data()));


    for (size_t i = 0; i < kMaxInFlightCount; i++)
    {
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer                 = m_uniformBuffers[i];
        buffer_info.offset                 = 0;
        buffer_info.range                  = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptor_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        descriptor_write.pNext                = nullptr;
        descriptor_write.dstSet               = m_descriptorSets[i];
        descriptor_write.dstBinding           = 0;
        descriptor_write.dstArrayElement      = 0;
        descriptor_write.descriptorCount      = 1;
        descriptor_write.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.pImageInfo           = nullptr;
        descriptor_write.pBufferInfo          = &buffer_info;
        descriptor_write.pTexelBufferView     = nullptr;

        vkUpdateDescriptorSets(m_device, 1, &descriptor_write, 0, nullptr);
    }
}

void Graphics::DestroyDescriptorSet()
{
    if (m_descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    }

    if (m_descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
    }
}

void Graphics::CreatePipeline()
{
    std::vector<VkShaderModule> shader_modules;
    {
        std::vector<std::byte> vert_shader_code = loadShaderCode("test.vert.spv");
        std::vector<std::byte> frag_shader_code = loadShaderCode("test.frag.spv");

        VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        info.pNext                    = nullptr;
        info.flags                    = 0;

        VkShaderModule shader_module;
        {
            info.codeSize = vert_shader_code.size();
            info.pCode    = reinterpret_cast<const uint32_t*>(vert_shader_code.data());

            VK_EXCEPT(vkCreateShaderModule(m_device, &info, nullptr, &shader_module));
            shader_modules.push_back(shader_module);
        }
        {
            info.codeSize = frag_shader_code.size();
            info.pCode    = reinterpret_cast<const uint32_t*>(frag_shader_code.data());

            VK_EXCEPT(vkCreateShaderModule(m_device, &info, nullptr, &shader_module));
            shader_modules.push_back(shader_module);
        }
    }


    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
    {
        VkPipelineShaderStageCreateInfo shader_stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        shader_stage_info.pNext                           = nullptr;
        shader_stage_info.flags                           = 0;
        shader_stage_info.pName                           = "main";
        shader_stage_info.pSpecializationInfo             = nullptr;

        {
            shader_stage_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
            shader_stage_info.module = shader_modules[0];

            shader_stage_infos.push_back(shader_stage_info);
        }
        {
            shader_stage_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            shader_stage_info.module = shader_modules[1];

            shader_stage_infos.push_back(shader_stage_info);
        }
    }


    std::vector<VkDynamicState>      dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_info   = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamic_info.pNext                              = nullptr;
    dynamic_info.flags                              = 0;
    dynamic_info.dynamicStateCount                  = (uint32_t)dynamic_states.size();
    dynamic_info.pDynamicStates                     = dynamic_states.data();


    const uint32_t binding = 0;

    VkVertexInputBindingDescription vertex_input_binding_desc{};
    vertex_input_binding_desc.binding   = binding;
    vertex_input_binding_desc.stride    = (uint32_t)m_vertexLayout.getStride();
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descs;
    m_vertexLayout.getAttributeDescs(binding, vertex_input_attribute_descs);

    VkPipelineVertexInputStateCreateInfo vertex_input_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertex_input_info.pNext                                = nullptr;
    vertex_input_info.flags                                = 0;
    vertex_input_info.vertexBindingDescriptionCount        = 1;
    vertex_input_info.pVertexBindingDescriptions           = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount      = (uint32_t)vertex_input_attribute_descs.size();
    vertex_input_info.pVertexAttributeDescriptions         = vertex_input_attribute_descs.data();


    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    input_assembly_info.pNext                                  = nullptr;
    input_assembly_info.flags                                  = 0;
    input_assembly_info.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable                 = VK_FALSE;


    VkViewport viewport{ .x        = 0.0f,
                         .y        = 0.0f,
                         .width    = (float)m_swapchainImageExtent.width,
                         .height   = (float)m_swapchainImageExtent.height,
                         .minDepth = 0.0f,
                         .maxDepth = 1.0f };
    VkRect2D   scissor{
          .offset = {0, 0},
          .extent = m_swapchainImageExtent
    };
    VkPipelineViewportStateCreateInfo viewport_info = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewport_info.pNext                             = nullptr;
    viewport_info.flags                             = 0;
    viewport_info.viewportCount                     = 1;
    viewport_info.pViewports                        = &viewport;
    viewport_info.scissorCount                      = 1;
    viewport_info.pScissors                         = &scissor;


    VkPipelineRasterizationStateCreateInfo rasterizeation_info = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizeation_info.pNext                                  = nullptr;
    rasterizeation_info.flags                                  = 0;
    rasterizeation_info.depthClampEnable                       = VK_FALSE;
    rasterizeation_info.rasterizerDiscardEnable                = VK_FALSE;
    rasterizeation_info.polygonMode                            = VK_POLYGON_MODE_FILL;
    rasterizeation_info.cullMode                               = VK_CULL_MODE_BACK_BIT;
    rasterizeation_info.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizeation_info.depthBiasEnable                        = VK_FALSE;
    rasterizeation_info.depthBiasConstantFactor                = 0.0f;
    rasterizeation_info.depthBiasClamp                         = 0.0f;
    rasterizeation_info.depthBiasSlopeFactor                   = 0.0f;
    rasterizeation_info.lineWidth                              = 1.0f;


    VkPipelineMultisampleStateCreateInfo multisampling_info = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling_info.pNext                                = nullptr;
    multisampling_info.flags                                = 0;
    multisampling_info.sampleShadingEnable                  = VK_FALSE;
    multisampling_info.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
    multisampling_info.minSampleShading                     = 1.0f;
    multisampling_info.pSampleMask                          = nullptr;
    multisampling_info.alphaToCoverageEnable                = VK_FALSE;
    multisampling_info.alphaToOneEnable                     = VK_FALSE;


    // VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};


    VkPipelineColorBlendAttachmentState color_blend_attachment{ .blendEnable         = VK_FALSE,
                                                                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                                                                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                                                                .colorBlendOp        = VK_BLEND_OP_ADD,
                                                                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                                                                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                                                .alphaBlendOp        = VK_BLEND_OP_ADD,
                                                                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };
    VkPipelineColorBlendStateCreateInfo color_blend_info = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    color_blend_info.pNext                               = nullptr;
    color_blend_info.flags                               = 0;
    color_blend_info.logicOpEnable                       = VK_FALSE;
    color_blend_info.logicOp                             = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount                     = 1;
    color_blend_info.pAttachments                        = &color_blend_attachment;
    color_blend_info.blendConstants[0]                   = 0.0f;
    color_blend_info.blendConstants[1]                   = 0.0f;
    color_blend_info.blendConstants[2]                   = 0.0f;
    color_blend_info.blendConstants[3]                   = 0.0f;


    VkPipelineLayoutCreateInfo pipeline_layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipeline_layout_info.pNext                      = nullptr;
    pipeline_layout_info.flags                      = 0;
    pipeline_layout_info.setLayoutCount             = 1;
    pipeline_layout_info.pSetLayouts                = &m_descriptorSetLayout;
    pipeline_layout_info.pushConstantRangeCount     = 0;
    pipeline_layout_info.pPushConstantRanges        = nullptr;

    VK_EXCEPT(vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipelineLayout));


    VkGraphicsPipelineCreateInfo pipeline_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipeline_info.pNext                        = nullptr;
    pipeline_info.flags                        = 0;
    pipeline_info.stageCount                   = (uint32_t)shader_stage_infos.size();
    pipeline_info.pStages                      = shader_stage_infos.data();
    pipeline_info.pVertexInputState            = &vertex_input_info;
    pipeline_info.pInputAssemblyState          = &input_assembly_info;
    pipeline_info.pTessellationState           = nullptr;
    pipeline_info.pViewportState               = &viewport_info;
    pipeline_info.pRasterizationState          = &rasterizeation_info;
    pipeline_info.pMultisampleState            = &multisampling_info;
    pipeline_info.pDepthStencilState           = nullptr;
    pipeline_info.pColorBlendState             = &color_blend_info;
    pipeline_info.pDynamicState                = &dynamic_info;
    pipeline_info.layout                       = m_pipelineLayout;
    pipeline_info.renderPass                   = m_renderPass;
    pipeline_info.subpass                      = 0;
    pipeline_info.basePipelineHandle           = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex            = -1;

    VK_EXCEPT(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphicsPipeline));


    for (auto sm : shader_modules)
    {
        vkDestroyShaderModule(m_device, sm, nullptr);
    }
}

void Graphics::DestroyPipeline()
{
    if (m_graphicsPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    }
    if (m_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    }
}
