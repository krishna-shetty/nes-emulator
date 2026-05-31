#ifndef NES_H
#define NES_H
#include "nes_utils.h"
#include "nes_bus.h"
#include "nes_cpu.h"
#include "nes_ppu.h"
#include "nes_cartridge.h"
#include <memory>
namespace NES
{
    class Emulator  
    {
    public:
        Emulator(const char* title, int width, int height);
        void insertCartridge(std::shared_ptr<Cartridge> cartridge);
        void tick(std::function<void()> callback = nullptr);
        void reset();

        Bus& getBus();
        CPU& getCPU();
        PPU& getPPU();
        bool isRunning() const;
    private:
        Bus _bus;
        CPU _cpu{_bus};
        PPU _ppu;

        uint64_t _lastTime{SDL_GetTicks()};
        double _accumulator{0.0};
 
        uint32_t _clockCounter{0};
    };
} // namespace NES
#endif // NES_H