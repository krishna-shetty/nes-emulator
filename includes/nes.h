#ifndef NES_H
#define NES_H

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
} // namespace NES
#endif // namespace NES_H