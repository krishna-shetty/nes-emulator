#ifndef NES_UTILS_H
#define NES_UTILS_H

#include <cstdint>

namespace NES
{
    enum class Region
    {
        NTSC,
        PAL
    };

    inline void setBit(uint8_t &num, uint8_t n)
    {
        num |= (1 << n);
    }

    inline void clearBit(uint8_t &num, uint8_t n)
    {
        num &= ~(1 << n);
    }

    inline void toggleBit(uint8_t &num, uint8_t n)
    {
        num ^= (1 << n);
    }

    inline uint8_t getBit(uint8_t num, uint8_t n)
    {
        uint8_t value = num >> n;
        value &= 1;
        return value;
    }

    // https://stackoverflow.com/a/2602885
    inline uint8_t reverseByte(uint8_t byte)
    {
        byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
        byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
        byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
        return byte;
    }
} // namespace NES
#endif // NES_UTILS_H