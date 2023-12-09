#include "graphics/bindable/vertex_buffer.h"

VertexBuffer::VertexBuffer(Graphics& gfx, const vertex::Buffer& vb)
    : m_size((VkDeviceSize)vb.sizeOf())
{
    create(gfx,
           m_size,
           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
           m_buffer,
           m_memory);


    VkBuffer       staging_buffer = {};
    VkDeviceMemory staging_memory = {};
    try
    {
        create(gfx,
               m_size,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               staging_buffer,
               staging_memory);

        {
            Mapper<std::byte> map(gfx, staging_memory, m_size, 0);
            std::memcpy(&map, vb.dataPtr(), m_size);
        }

        Buffer::copy(gfx, staging_buffer, m_buffer, m_size);

        vkDestroyBuffer(getDevice(gfx), staging_buffer, nullptr);
        vkFreeMemory(getDevice(gfx), staging_memory, nullptr);
    }
    catch (Graphics::VkException& e)
    {
        if (staging_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(getDevice(gfx), staging_buffer, nullptr);
        }
        if (staging_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(getDevice(gfx), staging_memory, nullptr);
        }
    }
}

void VertexBuffer::bind_impl(Graphics& gfx) noexcept
{
    vkCmdBindVertexBuffers(getCurrSwapchainCmd(gfx), 0, 1, &m_buffer, &m_offset);
}

void VertexBuffer::destroy_impl(Graphics& gfx) noexcept
{
    if (m_buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(getDevice(gfx), m_buffer, nullptr);
    }
    if (m_memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(getDevice(gfx), m_memory, nullptr);
    }
    resetToDefault();
}

void VertexBuffer::resetToDefault() noexcept
{
    m_size   = 0;
    m_buffer = VK_NULL_HANDLE;
    m_offset = 0;
    m_memory = VK_NULL_HANDLE;
}
