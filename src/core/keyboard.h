#pragma once
#include <bitset>
#include <optional>
#include <queue>

class Keyboard
{
    friend class App;

public:
    class Event
    {
    public:
        enum class Type
        {
            Press,
            Release,
        };

    private:
        Type type;
        int  code;

    public:
        Event(Type type, int code) noexcept
            : type(type)
            , code(code)
        {}

        bool isPress() const noexcept { return type == Type::Press; }
        bool isRelease() const noexcept { return type == Type::Release; }
        int  getCode() const noexcept { return code; }
    };

public:
    Keyboard()                           = default;
    Keyboard(const Keyboard&)            = delete;
    Keyboard& operator=(const Keyboard&) = delete;

    // key event stuff
    bool                 isKeyPressed(int keycode) const noexcept;
    std::optional<Event> readKey() noexcept;
    bool                 isKeyEmpty() const noexcept;
    void                 flushKey() noexcept;

    // char event stuff
    std::optional<unsigned int> readChar() noexcept;
    bool                        isCharEmpty() const noexcept;
    void                        flushChar() noexcept;

    void flush() noexcept;

private:
    void onKeyPressed(int keycode) noexcept;
    void onKeyReleased(int keycode) noexcept;
    void onChar(unsigned int character) noexcept;

    void clearState() noexcept;

    template <typename T>
    static void trimBuffer(std::queue<T>& buffer) noexcept;

private:
    static constexpr unsigned int k_max_key_count       = 256u;
    static constexpr unsigned int k_max_key_event_count = 16u;

    std::bitset<k_max_key_count> m_keystates;
    std::queue<Event>            m_keybuffer;
    std::queue<unsigned int>     m_charbuffer;
};
