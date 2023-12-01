#pragma once
#include <optional>
#include <queue>

class Mouse
{
    friend class App;

public:
    class Event
    {
    public:
        enum class Type
        {
            LPress,
            LRelease,
            RPress,
            RRelease,
            WheelUp,
            WheelDown,
            Move,
            Enter,
            Leave,
        };

    private:
        Type  type;
        bool  is_left_pressed;
        bool  is_right_pressed;
        float x;
        float y;

    public:
        Event(Type type, const Mouse& parent) noexcept
            : type(type)
            , is_left_pressed(parent.m_is_left_pressed)
            , is_right_pressed(parent.m_is_right_pressed)
            , x(parent.m_x)
            , y(parent.m_y)
        {}

        Type                    getType() const noexcept { return type; }
        std::pair<float, float> getPos() const noexcept { return { x, y }; }
        float                   getPosX() const noexcept { return x; }
        float                   getPosY() const noexcept { return y; }
        bool                    isLeftPressed() const noexcept { return is_left_pressed; }
        bool                    isRightPressed() const noexcept { return is_right_pressed; }
    };

public:
    Mouse()                        = default;
    Mouse(const Mouse&)            = delete;
    Mouse& operator=(const Mouse&) = delete;

    std::pair<float, float> getPos() const noexcept { return { m_x, m_y }; }
    float                   getPosX() const noexcept { return m_x; }
    float                   getPosY() const noexcept { return m_y; }

    bool isInWindow() const noexcept { return m_is_in_window; }
    bool isLeftPressed() const noexcept { return m_is_left_pressed; }
    bool isRightPressed() const noexcept { return m_is_right_pressed; }

    std::optional<Mouse::Event> read() noexcept;

    bool isEmpty() const noexcept { return m_buffer.empty(); }

    void flush() noexcept { m_buffer = {}; }

private:
    void onMouseMove(float x, float y) noexcept;
    void onMouseLeave() noexcept;
    void onMouseEnter() noexcept;
    void onLeftPressed() noexcept;
    void onLeftReleased() noexcept;
    void onRightPressed() noexcept;
    void onRightReleased() noexcept;
    void onWheelUp() noexcept;
    void onWheelDown() noexcept;
    void onWheelDelta(int delta) noexcept;

    void trimBuffer() noexcept;

private:
    static constexpr unsigned int k_max_event_count = 16u;

    float m_x;
    float m_y;
    bool  m_is_left_pressed   = false;
    bool  m_is_right_pressed  = false;
    bool  m_is_in_window      = false;
    int   m_wheel_delta_carry = 0;

    std::queue<Event> m_buffer;
};
