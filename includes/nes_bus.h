#ifndef NES_BUS_H
#define NES_BUS_H

#include "nes_ram.h"
#include <vector>
#include <cstdint>
#include "nes_cartridge.h"

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
        void loadCartridge(std::shared_ptr<Cartridge> cartridge);
        uint8_t peek(uint16_t address) const;

    private:
        RAM _ram;
        uint8_t _openBus{0};

        std::shared_ptr<Cartridge> _cartridge{nullptr};
    };
} // namespace NES

#endif // NES_BUS_H