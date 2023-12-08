#pragma once

#include "graphics/graphics_available.h"

class Buffer : public GraphicsAvailable
{
public:
    template <typename T>
    class Mapper
    {
    public:
        Mapper(Graphics& gfx, VkDeviceMemory memory, VkDeviceSize size, VkMemoryMapFlags flags)
            : m_gfx(gfx)
            , m_memory(memory)
            , m_data(static_cast<T*>(Buffer::map(m_gfx, m_memory, 0, size, flags)))
        {}
        Mapper(const Mapper&)            = delete;
        Mapper& operator=(const Mapper&) = delete;
        ~Mapper() noexcept { Buffer::unmap(m_gfx, m_memory); }

        T&       operator*() { return *m_data; }
        const T& operator*() const { return *m_data; }

        T&       operator[](size_t index) { return m_data[index]; }
        const T& operator[](size_t index) const { return m_data[index]; }

        T*       operator->() { return m_data; }
        const T* operator->() const { return m_data; }

        T*       operator&() { return m_data; }
        const T* operator&() const { return m_data; }

    private:
        Graphics&      m_gfx;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        T*             m_data   = nullptr;
    };

protected:
    Buffer() noexcept                = default;
    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;

protected:
    static void create(Graphics&             gfx,
                       VkDeviceSize          size,
                       VkBufferUsageFlags    usage,
                       VkMemoryPropertyFlags properties,
                       VkBuffer&             out_buffer,
                       VkDeviceMemory&       out_memory);
    static void copy(Graphics& gfx, VkBuffer src, VkBuffer dst, VkDeviceSize size);

    static void* map(Graphics& gfx, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags);
    static void  unmap(Graphics& gfx, VkDeviceMemory memory) noexcept;
};
