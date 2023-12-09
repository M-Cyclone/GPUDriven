#pragma once

#include "graphics/vertex.h"

#include "graphics/drawable/drawable.h"

class Box : public Drawable
{
public:
    Box(Graphics& gfx, vertex::Layout& layout);
    Box(const Box&)            = delete;
    Box& operator=(const Box&) = delete;
    virtual ~Box() noexcept    = default;

public:
    void      update(float dt, float tt) noexcept override;
    glm::mat4 getModelMatrix() const noexcept override;

protected:
    float m_total_time = 0.0f;
};
