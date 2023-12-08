#pragma once
#include <type_traits>

#include "graphics/graphics_available.h"

template <typename T>
class Bindable : public GraphicsAvailable
{
public:
    void bind(Graphics& gfx) noexcept { static_cast<T*>(this)->bind_impl(gfx); }
    void destroy(Graphics& gfx) noexcept { static_cast<T*>(this)->destroy_impl(gfx); }
};
