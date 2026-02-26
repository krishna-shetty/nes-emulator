#ifndef NES_RAM_H
#define NES_RAM_H
#include <cstdint>
#include <array>

namespace NES
{
    class RAM // Too many people going underground, Too many reaching for a piece of cake
    {
        private:
            std::array<uint8_t, 0x0800> _data;
        public:
            uint8_t read(uint16_t address);
            void write(uint16_t address, uint8_t value);
    };
} // namespace NES
#endif //NES_RAM_H
