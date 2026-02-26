#ifndef NES_BUS_H
#define NES_BUS_H

#include "nes_ram.h"

namespace NES
{
    class Bus
    {
        private:
            RAM* ram{nullptr};

            uint8_t openBus{0};

            uint8_t read(uint16_t address);
            void write(uint16_t address, uint8_t value);
    }; 
} //namespace NES
#endif // NES_BUS_H