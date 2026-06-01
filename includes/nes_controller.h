#ifndef NES_CONTROLLER_H
#define NES_CONTROLLER_H
#include <cstdint>
#include <SDL3/SDL.h>

namespace NES
{
    class Controller
    {
    public:
        enum class Button : uint8_t
        {
            A      = 0,
            B      = 1,
            Select = 2,
            Start  = 3,
            Up     = 4,
            Down   = 5,
            Left   = 6,
            Right  = 7
        };

        void strobe(uint8_t value);
        uint8_t read();
        void setButton(Button button, bool pressed);
        void handleEvent(const SDL_Event& event);

    private:
        uint8_t _state{0};    // current button state
        uint8_t _shifter{0};  // shift register for serial reads
        bool _strobe{false};  // strobe state
    };
}
#endif