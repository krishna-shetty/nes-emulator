#include "mappers/nes_mapper000.h"

using namespace NES;

Mapper000::Mapper000(uint8_t numPRGBanks, uint8_t numCHRBanks) : Mapper(numPRGBanks, numCHRBanks)
{

}

std::optional<uint32_t> Mapper000::cpuRead(uint16_t address)
{
    if (address >= 0x8000 && address <= 0xFFFF)
    {
        return static_cast<uint32_t>(address & (_numPRGBanks > 1 ? 0x7FFF : 0x3FFF));
    }
    return std::nullopt; // Return invalid address
}

std::optional<uint32_t> Mapper000::cpuWrite(uint16_t address)
{
    if (address >= 0x8000 && address <= 0xFFFF)
    {
        return static_cast<uint32_t>(address & (_numPRGBanks > 1 ? 0x7FFF : 0x3FFF));
    }
    return std::nullopt; // Return invalid address
}

std::optional<uint32_t> Mapper000::ppuRead(uint16_t address)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        return static_cast<uint32_t>(address);
    }
    return std::nullopt; // Return invalid address
}

std::optional<uint32_t> Mapper000::ppuWrite(uint16_t address)
{
    // Mapper 000 does not support PPU writes
    return std::nullopt;
}


