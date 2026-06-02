#include "nes.h"
#include "nes_controller.h"

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
        {
            if (_bus.getDMAInProgress())
            {
                if (_bus.getDMADummyCycle())
                {
                    if (_clockCounter % 2 == 1)
                    {
                        _bus.setDMADummyCycle(false);
                    }
                }
                else
                {
                    if (_clockCounter % 2 == 0)
                    {
                        uint16_t addr =
                            (static_cast<uint16_t>(_bus.getDMAPage()) << 8) |
                            _bus.getDMAAddress();

                        _bus.setDMAData(_bus.read(addr));
                    }
                    else
                    {
                        _ppu.setOAMData(_bus.getDMAAddress(), _bus.getDMAData());
                        _bus.incrementDMAAddress();

                        if (_bus.getDMAAddress() == 0x00)
                        {
                            _bus.setDMAInProgress(false);
                            _bus.setDMADummyCycle(true);
                        }
                    }
                }
            }
            else
            {
                _cpu.clock();
            }
        }

        if (_ppu.nmiRequested())
        {
            _cpu.NMI();
            _ppu.clearNMI();
        }

        _clockCounter++;
        _accumulator -= 1.0;
    }

    _ppu.clear();
    _ppu.draw();

    if (callback)
        callback();

    _ppu.present();
}

void Emulator::reset()
{
    _cpu.reset();
    _clockCounter = 0;
}

void Emulator::handleEvents(std::function<void(SDL_Event &)> eventHandler)
{
    _ppu.handleEvents([&](SDL_Event &event)
                      {
        if (eventHandler)
        {
            eventHandler(event);
        }
            
        _bus.getController1().handleEvent(event);
        _bus.getController2().handleEvent(event); });
}
