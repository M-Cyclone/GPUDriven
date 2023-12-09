#include "graphics/bindable/index_buffer.h"

template <typename T>
inline IndexBuffer::IndexBuffer(Graphics& gfx, const T* data, uint32_t count, VkIndexType type)
    : m_size(sizeof(T) * count)
    , m_count(count)
    , m_type(type)
{
    create(gfx,
           m_size,
           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
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
            std::memcpy(&map, data, m_size);
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

IndexBuffer::IndexBuffer(Graphics& gfx, std::span<const uint16_t> ib)
    : IndexBuffer(gfx, ib.data(), (uint32_t)ib.size(), VK_INDEX_TYPE_UINT16)
{}

IndexBuffer::IndexBuffer(Graphics& gfx, std::span<const uint32_t> ib)
    : IndexBuffer(gfx, ib.data(), (uint32_t)ib.size(), VK_INDEX_TYPE_UINT32)
{}

void IndexBuffer::bind_impl(Graphics& gfx) noexcept
{
    vkCmdBindIndexBuffer(getCurrSwapchainCmd(gfx), m_buffer, 0, m_type);
}

void IndexBuffer::destroy_impl(Graphics& gfx) noexcept
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

void IndexBuffer::resetToDefault() noexcept
{
    m_size   = 0;
    m_buffer = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
    m_count  = 0;
    m_type   = VK_INDEX_TYPE_NONE_KHR;
}
