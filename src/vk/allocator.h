#pragma once
#include <type_traits>

#include "vk/device.h"
#include "vk/resource.h"

class Allocator
{
public:
    explicit Allocator(Device& device)
        : m_device(device)
    {}

    Buffer createVertexBuffer(const vertex::Buffer& vb);

    template <typename T>
        requires std::is_integral_v<T>
    Buffer createIndexBuffer(const std::vector<T>& ib);

    Buffer createUniformBuffer(VkDeviceSize size);

    Buffer createStagingBuffer(VkDeviceSize size, const void* data);

    Buffer createStagingBuffer(std::span<const std::byte> data);

    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;

    void destroyBuffer(Buffer& buffer);

    Image createImage(uint32_t              width,
                      uint32_t              height,
                      VkFormat              format,
                      VkImageUsageFlags     usage,
                      VkMemoryPropertyFlags properties,
                      VkImageAspectFlags    aspect_flags,
                      uint32_t              mip_level = 1);
    
    void destroyImage(Image& image);

public:
    static void copyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, VkDeviceSize size);

    static void transitionImageLayout(VkCommandBuffer cmd,
                                      VkImage         image,
                                      VkImageLayout   old_layout,
                                      VkImageLayout   new_layout,
                                      uint32_t        mip_levels);

    static void copyBufferToImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst, uint32_t width, uint32_t height);

    uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

private:
    Device& m_device;
};

template <typename T>
    requires std::is_integral_v<T>
inline Buffer Allocator::createIndexBuffer(const std::vector<T>& ib)
{
    return createBuffer(static_cast<VkDeviceSize>(sizeof(T) * ib.size()),
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}
