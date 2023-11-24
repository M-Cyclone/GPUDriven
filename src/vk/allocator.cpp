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

void Allocator::destroyBuffer(Buffer& buffer)
{
    vkFreeMemory(m_device.device(), buffer.memory, nullptr);
    vkDestroyBuffer(m_device.device(), buffer.buffer, nullptr);

    buffer = {};
}

Image Allocator::createImage(uint32_t              width,
                             uint32_t              height,
                             VkFormat              format,
                             VkImageUsageFlags     usage,
                             VkMemoryPropertyFlags properties,
                             uint32_t              mip_level)
{
    Image img;

    VkImageCreateInfo img_info     = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    img_info.pNext                 = nullptr;
    img_info.flags                 = 0;
    img_info.imageType             = VK_IMAGE_TYPE_2D;
    img_info.format                = format;
    img_info.extent                = { width, height, 1 };
    img_info.mipLevels             = mip_level;
    img_info.arrayLayers           = 1;
    img_info.samples               = VK_SAMPLE_COUNT_1_BIT;
    img_info.tiling                = VK_IMAGE_TILING_OPTIMAL;
    img_info.usage                 = usage;
    img_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    img_info.queueFamilyIndexCount = 0;
    img_info.pQueueFamilyIndices   = nullptr;
    img_info.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    NVVK_CHECK(vkCreateImage(m_device.device(), &img_info, nullptr, &img.image));


    VkMemoryRequirements requirments;
    vkGetImageMemoryRequirements(m_device.device(), img.image, &requirments);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc_info.allocationSize       = requirments.size;
    alloc_info.memoryTypeIndex      = findMemoryType(requirments.memoryTypeBits, properties);

    NVVK_CHECK(vkAllocateMemory(m_device.device(), &alloc_info, nullptr, &img.memory));
    NVVK_CHECK(vkBindImageMemory(m_device.device(), img.image, img.memory, 0));


    VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    view_info.pNext                 = nullptr;
    view_info.flags                 = 0;
    view_info.image                 = img.image;
    view_info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format                = format;
    view_info.components            = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                        .a = VK_COMPONENT_SWIZZLE_IDENTITY };
    view_info.subresourceRange      = { .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                        .baseMipLevel   = 0,
                                        .levelCount     = 1,
                                        .baseArrayLayer = 0,
                                        .layerCount     = 1 };

    NVVK_CHECK(vkCreateImageView(m_device.device(), &view_info, nullptr, &img.view));


    return img;
}

void Allocator::destroyImage(Image& image)
{
    vkFreeMemory(m_device.device(), image.memory, nullptr);
    vkDestroyImageView(m_device.device(), image.view, nullptr);
    vkDestroyImage(m_device.device(), image.image, nullptr);

    image = {};
}

void Allocator::copyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size      = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);
}

void Allocator::transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext               = nullptr,
        .srcAccessMask       = 0,
        .dstAccessMask       = 0,
        .oldLayout           = old_layout,
        .newLayout           = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
        .subresourceRange    = {.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel   = 0,
                                .levelCount     = 1,
                                .baseArrayLayer = 0,
                                .layerCount     = 1}
    };

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void Allocator::copyBufferToImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst, uint32_t width, uint32_t height)
{
    VkBufferImageCopy region = {
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
        .imageOffset       = { .x = 0, .y = 0, .z = 0 },
        .imageExtent       = { .width = width, .height = height, .depth = 1 },
    };

    vkCmdCopyBufferToImage(cmd, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
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
