#include "core/keyboard.h"

bool Keyboard::isKeyPressed(int keycode) const noexcept
{
    return m_keystates[(size_t)keycode];
}

std::optional<Keyboard::Event> Keyboard::readKey() noexcept
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

bool Keyboard::isKeyEmpty() const noexcept
{
    return m_keybuffer.empty();
}

void Keyboard::flushKey() noexcept
{
    m_keybuffer = {};
}

std::optional<unsigned int> Keyboard::readChar() noexcept
{
    if (!m_charbuffer.empty())
    {
        char ch = m_charbuffer.front();
        m_charbuffer.pop();
        return ch;
    }
    else
    {
        return {};
    }
}

bool Keyboard::isCharEmpty() const noexcept
{
    return m_charbuffer.empty();
}

void Keyboard::flushChar() noexcept
{
    m_charbuffer = {};
}

void Keyboard::flush() noexcept
{
    flushKey();
    flushChar();
}

void Keyboard::onKeyPressed(int keycode) noexcept
{
    m_keystates[(size_t)keycode] = true;
    m_keybuffer.emplace(Keyboard::Event::Type::Press, keycode);
    trimBuffer(m_keybuffer);
}

void Keyboard::onKeyReleased(int keycode) noexcept
{
    m_keystates[(size_t)keycode] = false;
    m_keybuffer.emplace(Keyboard::Event::Type::Release, keycode);
    trimBuffer(m_keybuffer);
}

void Keyboard::onChar(unsigned int character) noexcept
{
    m_charbuffer.push(character);
    trimBuffer(m_charbuffer);
}

void Keyboard::clearState() noexcept
{
    m_keystates.reset();
}

template <typename T>
void Keyboard::trimBuffer(std::queue<T>& buffer) noexcept
{
    while (buffer.size() > k_max_key_event_count)
    {
        buffer.pop();
    }
}
