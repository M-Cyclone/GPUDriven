#include "buffer.h"

#include "graphics/graphics_throw_macros.h"

namespace
{
static uint32_t findMemoryType(VkPhysicalDevice gpu, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties available_properties;
    vkGetPhysicalDeviceMemoryProperties(gpu, &available_properties);

    for (uint32_t i = 0; i < available_properties.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i)) && (available_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    VK_EXCEPT(VK_INCOMPLETE);
    return ~0;
}
}  // namespace

void Buffer::create(Graphics&             gfx,
                    VkDeviceSize          size,
                    VkBufferUsageFlags    usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer&             out_buffer,
                    VkDeviceMemory&       out_memory)
{
    VkBufferCreateInfo buffer_info    = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.pNext                 = nullptr;
    buffer_info.flags                 = 0;
    buffer_info.size                  = size;
    buffer_info.usage                 = usage;
    buffer_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 0;
    buffer_info.pQueueFamilyIndices   = nullptr;

    VkBuffer buffer = {};
    VK_EXCEPT(vkCreateBuffer(getDevice(gfx), &buffer_info, nullptr, &buffer));


    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(getDevice(gfx), buffer, &requirements);

    VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocate_info.pNext                = nullptr;
    allocate_info.allocationSize       = requirements.size;
    allocate_info.memoryTypeIndex      = findMemoryType(getActiveGpu(gfx), requirements.memoryTypeBits, properties);

    VkDeviceMemory memory = {};
    VK_EXCEPT(vkAllocateMemory(getDevice(gfx), &allocate_info, nullptr, &memory));


    VK_EXCEPT(vkBindBufferMemory(getDevice(gfx), buffer, memory, 0));


    out_buffer = buffer;
    out_memory = memory;
}

void Buffer::copy(Graphics& gfx, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool                 = getSwapchainCmdPool(gfx);
    alloc_info.commandBufferCount          = 1;

    VkCommandBuffer cmd = {};
    VK_EXCEPT(vkAllocateCommandBuffers(getDevice(gfx), &alloc_info, &cmd));


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

    VK_EXCEPT(vkQueueSubmit(getQueueGraphics(gfx), 1, &submit_info, VK_NULL_HANDLE));
    VK_EXCEPT(vkQueueWaitIdle(getQueueGraphics(gfx)));


    vkFreeCommandBuffers(getDevice(gfx), getSwapchainCmdPool(gfx), 1, &cmd);
}

void* Buffer::map(Graphics& gfx, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags)
{
    void* data = {};
    VK_EXCEPT(vkMapMemory(getDevice(gfx), memory, offset, size, flags, &data));
    return data;
}

void Buffer::unmap(Graphics& gfx, VkDeviceMemory memory) noexcept
{
    vkUnmapMemory(getDevice(gfx), memory);
}
