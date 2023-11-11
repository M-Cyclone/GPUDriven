#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <vulkan/vulkan.h>

namespace nvvk
{
class DescriptorSetContainer;
struct GraphicsPipelineState;
struct GraphicsPipelineGenerator;
}  // namespace nvvk

class Swapchain;

class Device final
{
public:
    Device()                         = default;
    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&)                 = delete;
    void operator=(Device&&)         = delete;
    ~Device() noexcept               = default;

    void init();
    void deinit();

private:
    void createInstance();
    void destroyInstance();

    void createSurface();
    void destroySurface();

    void pickActiveGPU();
    void createDevice();
    void destroyDevice();

    // Vulkan tool funcs.
private:
    void queryQueueFamilyIndices();

    // Utils
public:
    void setVulkanApiVersion(uint32_t version) { m_vulkan_api_version = version; }

    // clang-format off
    void addInstanceLayer(const char* layer, bool is_optional = false)
    {
        m_instance_layers.emplace_back(layer, is_optional);
    }
    void removeInstanceLayer(const char* layer)
    {
        m_instance_layers.erase(std::find_if(m_instance_layers.begin(), m_instance_layers.end(), [layer](const auto& it)
        {
            return std::strcmp(it.first, layer) == 0;
        }));
    }
    void addDeviceExtension(const char* extension, bool is_optional = false)
    {
        m_device_extensions.emplace_back(extension, is_optional);
    }
    void removeDeviceExtension(const char* extension)
    {
        m_device_extensions.erase(std::find_if(m_device_extensions.begin(), m_device_extensions.end(), [extension](const auto& it)
        {
            return std::strcmp(it.first, extension) == 0;
        }));
    }
    // clang-format on

    void setCreateSurfaceFunc(std::function<VkResult(VkInstance, VkSurfaceKHR&)> func) { m_createSurface = func; }

private:
    void checkAvailableRequiredInstanceLayers();
    void checkAvailableRequiredInstanceExtensions();
    void checkAvailableRequiredDeviceExtensions();

public:
    VkSurfaceKHR     surface() const { return m_surface; }
    VkPhysicalDevice activeGPU() const { return m_active_gpu; }
    VkDevice         device() const { return m_device; }
    VkQueue          queueGraphics() const { return m_graphics_queue; }
    VkQueue          queuePresent() const { return m_present_queue; }

    auto     getUniqueQueueFamilyIndices() const -> std::vector<uint32_t>;
    uint32_t getQueueGraphicsIndex() const { return *m_queue_family_indices.graphics; }
    uint32_t getQueuePresentIndex() const { return *m_queue_family_indices.present; }

public:
    uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

    void createBuffer(VkDeviceSize          size,
                      VkBufferUsageFlags    usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer&             buffer,
                      VkDeviceMemory&       buffer_mem) const;

    void* mapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags) const;
    void  unmapMemory(VkDeviceMemory memory) const;

    void copyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, VkDeviceSize size) const;

private:
    // Vulkan configs.
    uint32_t    m_vulkan_api_version = VK_API_VERSION_1_3;
    std::string m_program_name;

    std::vector<std::pair<const char*, bool>> m_instance_layers;
    std::vector<std::pair<const char*, bool>> m_device_extensions;

    std::vector<const char*> m_enabled_instance_layers;
    std::vector<const char*> m_enabled_instance_extensions;
    std::vector<const char*> m_enabled_device_extensions;

    bool m_enable_validation_layers = false;

    VkInstance m_instance = VK_NULL_HANDLE;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT            m_debug_messenger                 = VK_NULL_HANDLE;
    PFN_vkCreateDebugUtilsMessengerEXT  m_vkCreateDebugUtilsMessengerEXT  = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT m_vkDestroyDebugUtilsMessengerEXT = nullptr;
#endif

    std::function<VkResult(VkInstance, VkSurfaceKHR&)> m_createSurface;
    VkSurfaceKHR                                       m_surface = VK_NULL_HANDLE;

    struct QueueFamilyIndices final
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;

        [[nodiscard]]
        bool isCompleted() const
        {
            return graphics.has_value() && present.has_value();
        }
    } m_queue_family_indices;
    VkPhysicalDevice m_active_gpu     = VK_NULL_HANDLE;
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkQueue          m_graphics_queue = VK_NULL_HANDLE;
    VkQueue          m_present_queue  = VK_NULL_HANDLE;

    bool m_is_initialized = false;
};
