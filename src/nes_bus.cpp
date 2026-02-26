#include "nes_bus.h"
#include "nes_not_implemented.h"

using namespace NES;

uint8_t Bus::read(uint16_t address)
{
    uint8_t data = openBus;

    // $0000-$1FFF : 2KB internal RAM, mirrored every $0800
    if (address < 0x2000)
    {
        data = ram->read(address & 0x07FF);
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
    // $4020-$FFFF cartridge use
    else
    {
        throw NotImplemented("TODO: Unmapped, available for cartridge use, catridge RAM, catridge ROM and mapper registers");
    }

    openBus = data; 

    return data;
}