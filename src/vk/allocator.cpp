#include "vk/allocator.h"
#include <stdexcept>

Buffer Allocator::createVertexBuffer(const vertex::Buffer& vb)
{
    return createBuffer(static_cast<VkDeviceSize>(vb.sizeOf()),
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

Buffer Allocator::createUniformBuffer(VkDeviceSize size)
{
    return createBuffer(size,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

Buffer Allocator::createStagingBuffer(VkDeviceSize size, const void* data)
{
    Buffer buffer =
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    {
        void* ptr = m_device.mapMemory(buffer.memory, 0, size, 0);
        std::memcpy(ptr, data, (size_t)size);
        m_device.unmapMemory(buffer.memory);
    }

    return buffer;
}

Buffer Allocator::createStagingBuffer(std::span<const std::byte> data)
{
    VkDeviceSize size = static_cast<VkDeviceSize>(data.size());

    Buffer buffer =
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    {
        void* ptr = m_device.mapMemory(buffer.memory, 0, size, 0);
        std::memcpy(ptr, data.data(), (size_t)size);
        m_device.unmapMemory(buffer.memory);
    }

    return buffer;
}

Buffer Allocator::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const
{
    Buffer buffer;

    VkBufferCreateInfo buffer_info    = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.pNext                 = nullptr;
    buffer_info.flags                 = 0;
    buffer_info.size                  = size;
    buffer_info.usage                 = usage;
    buffer_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 0;
    buffer_info.pQueueFamilyIndices   = nullptr;

    NVVK_CHECK(vkCreateBuffer(m_device.device(), &buffer_info, nullptr, &buffer.buffer));


    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(m_device.device(), buffer.buffer, &mem_requirements);


    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc_info.allocationSize       = mem_requirements.size;
    alloc_info.memoryTypeIndex      = findMemoryType(mem_requirements.memoryTypeBits, properties);

    NVVK_CHECK(vkAllocateMemory(m_device.device(), &alloc_info, nullptr, &buffer.memory));

    NVVK_CHECK(vkBindBufferMemory(m_device.device(), buffer.buffer, buffer.memory, 0));

    return buffer;
}

void Allocator::copyBuffer(VkCommandBuffer cmd, Buffer& src, Buffer& dst, VkDeviceSize size)
{
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    NVVK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size      = size;
        vkCmdCopyBuffer(cmd, src.buffer, dst.buffer, 1, &copyRegion);
    }
    NVVK_CHECK(vkEndCommandBuffer(cmd));
}

void Allocator::destroyBuffer(Buffer& buffer)
{
    vkFreeMemory(m_device.device(), buffer.memory, nullptr);
    vkDestroyBuffer(m_device.device(), buffer.buffer, nullptr);

    buffer = {};
}

uint32_t Allocator::findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(m_device.activeGPU(), &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i)) && ((mem_props.memoryTypes[i].propertyFlags & properties) == properties))
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}
