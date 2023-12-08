#pragma once
#include <type_traits>
#include <span>

#include "graphics/bindable/bindable.h"

#include "graphics/resource/buffer.h"

class IndexBuffer : public Buffer,
                    public Bindable<IndexBuffer>
{
    friend class Bindable<IndexBuffer>;

public:
    IndexBuffer(Graphics& gfx, std::span<const uint16_t> ib);
    IndexBuffer(Graphics& gfx, std::span<const uint32_t> ib);
    IndexBuffer(const IndexBuffer&)            = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

private:
    template <typename T>
    IndexBuffer(Graphics& gfx, const T* data, uint32_t count, VkIndexType type);

public:
    uint32_t getCount() const { return m_count; }

private:
    void bind_impl(Graphics& gfx) noexcept;
    void destroy_impl(Graphics& gfx) noexcept;

protected:
    VkDeviceSize   m_size   = 0;
    VkBuffer       m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    uint32_t       m_count  = 0;
    VkIndexType    m_type   = VK_INDEX_TYPE_NONE_KHR;
};
