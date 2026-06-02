#include "nes_controller.h"
using namespace NES;

void Controller::strobe(uint8_t value)
{
    _strobe = value & 0x01;
    if (_strobe)
        _shifter = _state; // latch current state
}

uint8_t Controller::read()
{
    if (_strobe)
        return _state & 0x01;

    uint8_t bit = _shifter & 0x01;
    _shifter >>= 1;

    return bit;
}

void Controller::setButton(Button button, bool pressed)
{
    uint8_t bit = static_cast<uint8_t>(button);
    if (pressed)
        _state |= (1 << bit);
    else
        _state &= ~(1 << bit);
}

void Controller::handleEvent(const SDL_Event& event)
{
    if (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP)
        return;

    bool pressed = event.type == SDL_EVENT_KEY_DOWN;

    switch (event.key.key)
    {
    case SDLK_Z:         setButton(Button::A,      pressed); break;
    case SDLK_X:         setButton(Button::B,      pressed); break;
    case SDLK_RSHIFT:    setButton(Button::Select, pressed); break;
    case SDLK_RETURN:    setButton(Button::Start,  pressed); break;
    case SDLK_UP:        setButton(Button::Up,     pressed); break;
    case SDLK_DOWN:      setButton(Button::Down,   pressed); break;
    case SDLK_LEFT:      setButton(Button::Left,   pressed); break;
    case SDLK_RIGHT:     setButton(Button::Right,  pressed); break;
    }
}