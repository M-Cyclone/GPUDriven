#pragma once
#include <vector>
#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/bindable/vertex_buffer.h"
#include "graphics/bindable/index_buffer.h"

class Drawable
{
public:
    Drawable() noexcept                  = default;
    Drawable(const Drawable&)            = delete;
    Drawable& operator=(const Drawable&) = delete;
    virtual ~Drawable() noexcept         = default;

public:
    void draw(class Graphics& gfx) const noexcept;

    void destroy(class Graphics& gfx) noexcept;

    virtual void      update(float dt, float tt) noexcept = 0;
    virtual glm::mat4 getModelMatrix() const noexcept     = 0;

protected:
    void setVertexBuffer(std::unique_ptr<VertexBuffer> vb) noexcept;
    void setIndexBuffer(std::unique_ptr<IndexBuffer> ib) noexcept;

private:
    std::unique_ptr<VertexBuffer> m_vertex_buffer;
    std::unique_ptr<IndexBuffer>  m_index_buffer;
};
