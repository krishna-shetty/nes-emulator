#include "nes_ram.h"

using namespace NES;

uint8_t RAM::read(uint16_t address)
{
    #ifdef NES_DEBUG
        if(address >= 0x0800)
            throw std::runtime_error("RAM out of bounds");
    #endif
    
    return _data[address];
}

void RAM::write(uint16_t address, uint8_t value)
{
    #ifdef NES_DEBUG
        if(address >= 0x0800)
            throw std::runtime_error("RAM out of bounds");
    #endif

    _data[address] = value;
}