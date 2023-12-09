#pragma once

#include "graphics/vertex.h"

#include "graphics/bindable/bindable.h"

#include "graphics/resource/buffer.h"

class VertexBuffer : public Buffer,
                     public Bindable<VertexBuffer>
{
    friend class Bindable<VertexBuffer>;

public:
    VertexBuffer(Graphics& gfx, const vertex::Buffer& vb);
    VertexBuffer(const VertexBuffer&)            = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

private:
    void bind_impl(Graphics& gfx) noexcept;
    void destroy_impl(Graphics& gfx) noexcept;

    void resetToDefault() noexcept;

protected:
    VkDeviceSize   m_size   = 0;
    VkBuffer       m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize   m_offset = 0;
};
