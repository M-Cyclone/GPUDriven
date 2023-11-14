#include "vk/device.h"
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "utils/log.h"

#include "vk/error.h"
#include "vk/swapchain.h"
#include "vk/descriptorsets.h"
#include "vk/pipeline.h"

void Device::init()
{
    if (m_is_initialized)
    {
        return;
    }

    createInstance();

    createSurface();

    pickActiveGPU();
    queryQueueFamilyIndices();
    createDevice();

    m_is_initialized = true;
}

void Device::deinit()
{
    if (!m_is_initialized)
    {
        return;
    }

    destroyDevice();

    destroySurface();
    destroyInstance();

    m_is_initialized = false;
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

void Device::createInstance()
{
    // clang-format off
    m_enable_validation_layers = std::find_if(m_instance_layers.begin(), m_instance_layers.end(), [](const auto& it)
    {
        return std::strcmp(it.first, "VK_LAYER_KHRONOS_validation") == 0;
    }) != m_instance_layers.end();
    // clang-format on


#ifdef USE_VULKAN_VALIDATION_LAYER
    VkDebugUtilsMessengerCreateInfoEXT debug_info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = debugCallback;
    debug_info.pUserData       = nullptr;
#endif  // USE_VULKAN_VALIDATION_LAYER


    VkApplicationInfo app_info  = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pNext              = nullptr;
    app_info.pApplicationName   = m_program_name.c_str();
    app_info.applicationVersion = 1;
    app_info.pEngineName        = m_program_name.c_str();
    app_info.engineVersion      = 1;
    app_info.apiVersion         = m_vulkan_api_version;

    checkAvailableRequiredInstanceLayers();
    checkAvailableRequiredInstanceExtensions();

    VkInstanceCreateInfo info    = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    info.pNext                   = nullptr;
    info.flags                   = 0;
    info.pApplicationInfo        = &app_info;
    info.enabledLayerCount       = static_cast<uint32_t>(m_enabled_instance_layers.size());
    info.ppEnabledLayerNames     = m_enabled_instance_layers.empty() ? nullptr : m_enabled_instance_layers.data();
    info.enabledExtensionCount   = static_cast<uint32_t>(m_enabled_instance_extensions.size());
    info.ppEnabledExtensionNames = m_enabled_instance_extensions.empty() ? nullptr : m_enabled_instance_extensions.data();

#ifdef USE_VULKAN_VALIDATION_LAYER
    if (m_enable_validation_layers)
    {
        info.pNext = &debug_info;
    }
#endif  // USE_VULKAN_VALIDATION_LAYER


    NVVK_CHECK(vkCreateInstance(&info, nullptr, &m_instance));


#ifdef USE_VULKAN_VALIDATION_LAYER
    m_vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    m_vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (m_vkCreateDebugUtilsMessengerEXT && m_vkDestroyDebugUtilsMessengerEXT)
    {
        NVVK_CHECK(m_vkCreateDebugUtilsMessengerEXT(m_instance, &debug_info, nullptr, &m_debug_messenger));
    }
#endif  // USE_VULKAN_VALIDATION_LAYER
}

void Device::destroyInstance()
{
#ifdef USE_VULKAN_VALIDATION_LAYER
    if (m_vkCreateDebugUtilsMessengerEXT && m_vkDestroyDebugUtilsMessengerEXT)
    {
        m_vkDestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
    }
#endif  // USE_VULKAN_VALIDATION_LAYER
    vkDestroyInstance(m_instance, nullptr);
}

void Device::createSurface()
{
    NVVK_CHECK(m_createSurface(m_instance, m_surface));
}

void Device::destroySurface()
{
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

void Device::pickActiveGPU()
{
    uint32_t count;
    NVVK_CHECK(vkEnumeratePhysicalDevices(m_instance, &count, nullptr));
    std::vector<VkPhysicalDevice> gpus(count);
    NVVK_CHECK(vkEnumeratePhysicalDevices(m_instance, &count, gpus.data()));
    m_active_gpu = gpus[0];
}

void Device::createDevice()
{
    float priorities[] = { 1.0f };

    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    {
        std::set<uint32_t> queue_indices = { *m_queue_family_indices.graphics, *m_queue_family_indices.present };
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

    checkAvailableRequiredDeviceExtensions();

    VkDeviceCreateInfo info      = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    info.pNext                   = nullptr;
    info.flags                   = 0;
    info.queueCreateInfoCount    = static_cast<uint32_t>(queue_infos.size());
    info.pQueueCreateInfos       = queue_infos.data();
    info.enabledLayerCount       = 0;        // deprecated
    info.ppEnabledLayerNames     = nullptr;  // deprecated
    info.enabledExtensionCount   = static_cast<uint32_t>(m_enabled_device_extensions.size());
    info.ppEnabledExtensionNames = m_enabled_device_extensions.empty() ? nullptr : m_enabled_device_extensions.data();
    info.pEnabledFeatures        = nullptr;
    NVVK_CHECK(vkCreateDevice(m_active_gpu, &info, nullptr, &m_device));

    vkGetDeviceQueue(m_device, *m_queue_family_indices.graphics, 0, &m_graphics_queue);
    vkGetDeviceQueue(m_device, *m_queue_family_indices.present, 0, &m_present_queue);
}

void Device::destroyDevice()
{
    vkDestroyDevice(m_device, nullptr);
}

// Vulkan tool funcs
void Device::queryQueueFamilyIndices()
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_active_gpu, &count, nullptr);
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_active_gpu, &count, props.data());

    for (uint32_t i = 0; i < count; ++i)
    {
        const VkQueueFamilyProperties& prop = props[i];

        if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_queue_family_indices.graphics = i;
        }

        VkBool32 supported = VK_FALSE;
        NVVK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(m_active_gpu, i, m_surface, &supported));
        if (supported == VK_TRUE)
        {
            m_queue_family_indices.present = i;
        }

        if (m_queue_family_indices.isCompleted())
        {
            break;
        }
    }
}

// Utils
void Device::checkAvailableRequiredInstanceLayers()
{
    uint32_t count;
    NVVK_CHECK(vkEnumerateInstanceLayerProperties(&count, nullptr));
    std::vector<VkLayerProperties> available_layers(count);
    NVVK_CHECK(vkEnumerateInstanceLayerProperties(&count, available_layers.data()));

    std::vector<const char*> final_layers;
    std::vector<const char*> ignored_layers;
    std::vector<const char*> unavailable_layers;
    for (auto [required_layer, is_optional] : m_instance_layers)
    {
        bool found = false;
        for (const VkLayerProperties& prop : available_layers)
        {
            if (std::strcmp(required_layer, prop.layerName) == 0)
            {
                final_layers.push_back(required_layer);
                found = true;
                break;
            }
        }

        if (!found)
        {
            if (is_optional)
            {
                ignored_layers.push_back(required_layer);
            }
            else
            {
                unavailable_layers.push_back(required_layer);
            }
        }
    }

    if (!ignored_layers.empty())
    {
        std::ostringstream oss;
        oss << "Ignored the following instance layers:\n";
        for (const char* layer : ignored_layers)
        {
            oss << layer << "\n";
        }
        spdlog::info("{}", oss.str());
    }

    if (!unavailable_layers.empty())
    {
        std::ostringstream oss;
        oss << "Failed to aquire the following instance layers:\n";
        for (const char* layer : unavailable_layers)
        {
            oss << layer << "\n";
        }
        throw std::runtime_error(oss.str());
    }

    m_enabled_instance_layers = final_layers;
}

void Device::checkAvailableRequiredInstanceExtensions()
{
    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector  extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (m_enable_validation_layers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    m_enabled_instance_extensions = extensions;
}

void Device::checkAvailableRequiredDeviceExtensions()
{
    std::vector<const char*> final_exts;
    std::vector<const char*> ignored_exts;
    std::vector<const char*> unavailable_exts;

    uint32_t count;
    NVVK_CHECK(vkEnumerateDeviceExtensionProperties(m_active_gpu, nullptr, &count, nullptr));
    std::vector<VkExtensionProperties> available_exts(count);
    NVVK_CHECK(vkEnumerateDeviceExtensionProperties(m_active_gpu, nullptr, &count, available_exts.data()));

    for (auto [required_ext, is_optional] : m_device_extensions)
    {
        bool found = false;
        for (const VkExtensionProperties& prop : available_exts)
        {
            if (std::strcmp(required_ext, prop.extensionName) == 0)
            {
                final_exts.push_back(required_ext);
                found = true;
                break;
            }
        }

        if (!found)
        {
            if (is_optional)
            {
                ignored_exts.push_back(required_ext);
            }
            else
            {
                unavailable_exts.push_back(required_ext);
            }
        }
    }


    if (!ignored_exts.empty())
    {
        std::ostringstream oss;
        oss << "Ignored the following device extensions:\n";
        for (const char* layer : ignored_exts)
        {
            oss << layer << "\n";
        }
        spdlog::info("{}", oss.str());
    }

    if (!unavailable_exts.empty())
    {
        std::ostringstream oss;
        oss << "Failed to aquire the following device extensions:\n";
        for (const char* layer : unavailable_exts)
        {
            oss << layer << "\n";
        }
        throw std::runtime_error(oss.str());
    }

    m_enabled_device_extensions = final_exts;
}

auto Device::getUniqueQueueFamilyIndices() const -> std::vector<uint32_t>
{
    std::set    queues = { *m_queue_family_indices.graphics, *m_queue_family_indices.present };
    std::vector unique_queues(queues.begin(), queues.end());
    return unique_queues;
}

void* Device::mapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags) const
{
    void* data;
    NVVK_CHECK(vkMapMemory(m_device, memory, offset, size, flags, &data));
    return data;
}

void Device::unmapMemory(VkDeviceMemory memory) const
{
    vkUnmapMemory(m_device, memory);
}
