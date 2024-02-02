#include "core/keyboard.h"

bool Keyboard::IsKeyPressed(SDL_Scancode keycode) const noexcept
{
    return m_keystates[keycode];
}

std::optional<Keyboard::Event> Keyboard::ReadKey() noexcept
{
    if (!m_keybuffer.empty())
    {
        Keyboard::Event e = m_keybuffer.front();
        m_keybuffer.pop();
        return e;
    }
    else
    {
        return {};
    }
}

bool Keyboard::IsKeyEmpty() const noexcept
{
    return m_keybuffer.empty();
}

void Keyboard::Flush() noexcept
{
    m_keybuffer = {};
}

void Keyboard::OnKeyPressed(SDL_KeyboardEvent e) noexcept
{
    m_keystates[e.keysym.scancode] = true;
    Event& keyEvent                         = m_keybuffer.emplace(e);
    TrimBuffer(m_keybuffer);
}

void Keyboard::OnKeyReleased(SDL_KeyboardEvent e) noexcept
{
    m_keystates[e.keysym.scancode] = false;
    m_keybuffer.emplace(e);
    TrimBuffer(m_keybuffer);
}

void Keyboard::ClearState() noexcept
{
    m_keystates.reset();
}

template <typename T>
void Keyboard::TrimBuffer(std::queue<T>& buffer) noexcept
{
    while (buffer.size() > kMaxKeyEventCount)
    {
        buffer.pop();
    }
}
