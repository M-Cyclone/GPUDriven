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

    static void copyBuffer(VkCommandBuffer cmd, Buffer& src, Buffer& dst, VkDeviceSize size);

    void destroyBuffer(Buffer& buffer);

    uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

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
