#include "nes.h"

using namespace NES;

Emulator::Emulator(const char *title, int width, int height)
    : _ppu(title, width, height)
{
    _bus.connectPPU(&_ppu);
}

Bus &Emulator::getBus()
{
    return _bus;
}

CPU &Emulator::getCPU()
{
    return _cpu;
}

PPU &Emulator::getPPU()
{
    return _ppu;
}

bool Emulator::isRunning() const
{
    return _ppu.isRunning();
}

void Emulator::insertCartridge(std::shared_ptr<Cartridge> cartridge)
{
    _bus.insertCartridge(cartridge);
    _ppu.insertCartridge(cartridge);
}

void Emulator::tick(std::function<void()> callback)
{
    uint64_t now = SDL_GetTicks();
    double elapsed = (now - _lastTime) / 1000.0; // Convert milliseconds to seconds
    _lastTime = now;
    _accumulator += elapsed * (_cpu.getClockFrequency() * 3.0);

    while (_accumulator >= 1.0)
    {
        _ppu.clock();

        if (_clockCounter % 3 == 0)
            _cpu.clock();

        _clockCounter++;
        _accumulator -= 1.0;
    }

    _ppu.clear();
    if (callback)
        callback();
    _ppu.present();
}

void Emulator::reset()
{
    _cpu.reset();
    _clockCounter = 0;
}
