#include "nes_bus.h"
#include "nes_not_implemented.h"

using namespace NES;

uint8_t Bus::read(uint16_t address)
{
    uint8_t data = _openBus;

    // $0000-$1FFF : 2KB internal RAM, mirrored every $0800
    if (address < 0x2000)
    {
        data = _ram.read(address & 0x07FF);
    }
    // $2000-$3FFF : PPU registers, mirrored every 8 bytes
    else if (address >= 0x2000 && address < 0x4000)
    {
        throw NotImplemented("TODO: NES PPU Registers and mirrors of $2000-$2007", __func__);
    }
    // $4000-$4017 : APU and I/O registers
    else if (address >= 0x4000 && address < 0x4018)
    {
        throw NotImplemented("TODO: NES APU and I/O registers", __func__);
    }
    // $4018-$401F : APU and I/O functionality that is normally disabled
    else if (address >= 0x4018 && address < 0x4020)
    {
        throw NotImplemented("TODO: See CPU Test Mode on nesdev.org", __func__);
    }
    // $4020-$FFFF : cartridge use
    else
    {
        throw NotImplemented("TODO: Unmapped, available for cartridge use, cartridge RAM, cartridge ROM and mapper registers");
    }

    _openBus = data;
    return data;
}

void Bus::write(uint16_t address, uint8_t value)
{
    // $0000-$1FFF : 2KB internal RAM, mirrored every $0800
    if (address < 0x2000)
    {
        _ram.write(address & 0x07FF, value);
    }
    // $2000-$3FFF : PPU registers, mirrored every 8 bytes
    else if (address >= 0x2000 && address < 0x4000)
    {
        throw NotImplemented("TODO: NES PPU Registers and mirrors of $2000-$2007", __func__);
    }
    // $4000-$4017 : APU and I/O registers
    else if (address >= 0x4000 && address < 0x4018)
    {
        throw NotImplemented("TODO: NES APU and I/O registers", __func__);
    }
    // $4018-$401F : APU and I/O functionality that is normally disabled
    else if (address >= 0x4018 && address < 0x4020)
    {
        throw NotImplemented("TODO: See CPU Test Mode on nesdev.org", __func__);
    }
    // $4020-$FFFF : cartridge use
    else
    {
        throw NotImplemented("TODO: Unmapped, available for cartridge use, cartridge RAM, cartridge ROM and mapper registers");
    }
}

void Bus::loadProgram(uint16_t address, std::vector<uint8_t> const& bytes)
{
    for (size_t i = 0; i < bytes.size(); i++)
    {
        _ram.write((address + i) & 0x07FF, bytes[i]);
    }
}

uint8_t Bus::peek(uint16_t address) const
{
    if (address < 0x2000)
        return _ram.read(address & 0x07FF);
    if (address >= 0x8000) // cartridge ROM
    {
        auto data = _cartridge->cpuRead(address);
        if (data) return *data;
    }
    return 0; 
}

void Bus::insertCartridge(std::shared_ptr<Cartridge> cartridge)
{
    _cartridge = cartridge;
}