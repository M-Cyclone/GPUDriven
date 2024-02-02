#pragma once
#include <bitset>
#include <optional>
#include <queue>

#include <SDL.h>

class Keyboard
{
    friend class App;

public:
    class Event
    {
    public:
        enum class Type : uint8_t
        {
            Pressed,
            Released,
        };

    private:
        Type         type;
        uint8_t      repeat;
        uint16_t     mod; /**< current key modifiers */
        uint32_t     timestamp;
        SDL_Keycode  keycode;
        SDL_Scancode scancode;

    public:
        explicit Event(SDL_KeyboardEvent e) noexcept
            : type((e.type == SDL_KEYDOWN) ? Type::Pressed : Type::Released)
            , repeat(e.repeat)
            , mod(e.keysym.mod)
            , timestamp(e.timestamp)
            , keycode(e.keysym.sym)
            , scancode(e.keysym.scancode)
        {}

        bool         IsPressed() const noexcept { return type == Type::Pressed; }
        bool         IsReleased() const noexcept { return type == Type::Released; }
        uint8_t      GetRepeat() const noexcept { return repeat; }
        uint16_t     GetKeyModifiers() const noexcept { return mod; }
        uint32_t     GetTimestamp() const noexcept { return timestamp; }
        SDL_Keycode  GetKeyCode() const noexcept { return keycode; }
        SDL_Scancode GetScanCode() const noexcept { return scancode; }
    };

public:
    Keyboard()                           = default;
    Keyboard(const Keyboard&)            = delete;
    Keyboard& operator=(const Keyboard&) = delete;

    // key event stuff
    bool                 IsKeyPressed(SDL_Scancode keycode) const noexcept;
    bool                 IsKeyEmpty() const noexcept;
    std::optional<Event> ReadKey() noexcept;

    void Flush() noexcept;

private:
    void OnKeyPressed(SDL_KeyboardEvent e) noexcept;
    void OnKeyReleased(SDL_KeyboardEvent e) noexcept;

    void ClearState() noexcept;

    template <typename T>
    static void TrimBuffer(std::queue<T>& buffer) noexcept;

private:
    static constexpr unsigned int kMaxKeyEventCount = 16u;

    std::bitset<SDL_NUM_SCANCODES> m_keystates;
    std::queue<Event>              m_keybuffer;
};
