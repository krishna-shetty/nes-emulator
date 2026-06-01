#ifndef NES_BUS_H
#define NES_BUS_H

#include "nes_ram.h"
#include <vector>
#include <cstdint>
#include "nes_cartridge.h"
#include "nes_ppu.h"

namespace NES
{
    class Bus
    {
    public:
        Bus() = default;

        Bus(const Bus &) = delete;
        Bus &operator=(const Bus &) = delete;
        Bus(Bus &&) = delete;
        Bus &operator=(Bus &&) = delete;

        uint8_t read(uint16_t address);
        void write(uint16_t address, uint8_t value);

        void loadProgram(uint16_t address, std::vector<uint8_t> const &bytes);
        void insertCartridge(std::shared_ptr<Cartridge> cartridge);
        uint8_t peek(uint16_t address) const;

        void connectPPU(PPU* ppu);

    private:
        RAM _ram;
        uint8_t _openBus{0};

        PPU* _ppu{nullptr};

        std::shared_ptr<Cartridge> _cartridge{nullptr};
    };
} // namespace NES

#endif // NES_BUS_H