#pragma once

#include "graphics/resource/buffer.h"

template <typename T>
class UniformBuffer : public Buffer
{
public:
    UniformBuffer(Graphics& gfx, const T& data)
        : m_size((VkDeviceSize)(sizeof(UniformBufferObject)))
    {
        create(gfx,
               m_size,
               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               m_buffer,
               m_memory);
    }
    UniformBuffer(const UniformBuffer&)            = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    Mapper<T> makeMapper(Graphics& gfx) { return Mapper<T>(gfx, m_memory, m_size, 0); }

    VkDescriptorBufferInfo makeInfo(VkDeviceSize offset) const
    {
        return VkDescriptorBufferInfo{ .buffer = m_buffer, .offset = offset, .range = sizeof(T) };
    }

    void destroy(Graphics& gfx) noexcept
    {
        m_size = 0;
        if (m_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(getDevice(gfx), m_buffer, nullptr);
        }
        if (m_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(getDevice(gfx), m_memory, nullptr);
        }
    }

protected:
    VkDeviceSize   m_size   = 0;
    VkBuffer       m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
};
