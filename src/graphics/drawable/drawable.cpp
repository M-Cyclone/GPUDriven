#include "graphics/drawable/drawable.h"

void Drawable::draw(Graphics& gfx) const noexcept
{
    if (m_vertex_buffer)
    {
        m_vertex_buffer->bind(gfx);
    }
    if (m_index_buffer)
    {
        m_index_buffer->bind(gfx);
    }
    gfx.drawIndexed(m_index_buffer->getCount());
}

void Drawable::destroy(Graphics& gfx) noexcept
{
    if (m_vertex_buffer)
    {
        m_vertex_buffer->destroy(gfx);
        m_vertex_buffer.reset();
    }
    if (m_index_buffer)
    {
        m_index_buffer->destroy(gfx);
        m_index_buffer.reset();
    }
}

void Drawable::setVertexBuffer(std::unique_ptr<VertexBuffer> vb) noexcept
{
    m_vertex_buffer = std::move(vb);
}

void Drawable::setIndexBuffer(std::unique_ptr<IndexBuffer> ib) noexcept
{
    m_index_buffer = std::move(ib);
}
