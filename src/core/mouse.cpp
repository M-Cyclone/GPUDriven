#include "core/mouse.h"

std::optional<Mouse::Event> Mouse::read() noexcept
{
    if (!m_buffer.empty())
    {
        Mouse::Event e = m_buffer.front();
        m_buffer.pop();
        return e;
    }
    else
    {
        return {};
    }
}

void Mouse::onMouseMove(float x, float y) noexcept
{
    m_x = x;
    m_y = y;

    m_buffer.emplace(Mouse::Event::Type::Move, *this);
    trimBuffer();
}

void Mouse::onMouseLeave() noexcept
{
    m_is_in_window = false;

    m_buffer.emplace(Mouse::Event::Type::Leave, *this);
    trimBuffer();
}

void Mouse::onMouseEnter() noexcept
{
    m_is_in_window = true;

    m_buffer.emplace(Mouse::Event::Type::Enter, *this);
    trimBuffer();
}

void Mouse::onLeftPressed() noexcept
{
    m_is_left_pressed = true;

    m_buffer.emplace(Mouse::Event::Type::LPress, *this);
    trimBuffer();
}

void Mouse::onLeftReleased() noexcept
{
    m_is_left_pressed = false;

    m_buffer.emplace(Mouse::Event::Type::LRelease, *this);
    trimBuffer();
}

void Mouse::onRightPressed() noexcept
{
    m_is_right_pressed = true;

    m_buffer.emplace(Mouse::Event::Type::RPress, *this);
    trimBuffer();
}

void Mouse::onRightReleased() noexcept
{
    m_is_right_pressed = false;

    m_buffer.emplace(Mouse::Event::Type::RRelease, *this);
    trimBuffer();
}

void Mouse::onWheelUp() noexcept
{
    m_buffer.push(Mouse::Event(Mouse::Event::Type::WheelUp, *this));
    trimBuffer();
}

void Mouse::onWheelDown() noexcept
{
    m_buffer.push(Mouse::Event(Mouse::Event::Type::WheelDown, *this));
    trimBuffer();
}

void Mouse::onWheelDelta(int delta) noexcept
{
    // generate events for every 120
    constexpr int k_delta_trigger = 120;

    m_wheel_delta_carry += delta;

    while (m_wheel_delta_carry >= k_delta_trigger)
    {
        m_wheel_delta_carry -= k_delta_trigger;
        onWheelUp();
    }
    while (m_wheel_delta_carry <= -k_delta_trigger)
    {
        m_wheel_delta_carry += k_delta_trigger;
        onWheelDown();
    }
}

void Mouse::trimBuffer() noexcept
{
    while (m_buffer.size() > k_max_event_count)
    {
        m_buffer.pop();
    }
}
