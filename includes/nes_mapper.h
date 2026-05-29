#ifndef NES_MAPPER_H
#define NES_MAPPER_H

#include <cstdint>

namespace NES
{
    class Mapper
    {
    public:
        Mapper(uint8_t numPRGBanks, uint8_t numCHRBanks);
       virtual ~Mapper() = default;

        virtual uint8_t read(uint16_t address) = 0;
        virtual void write(uint16_t address, uint8_t value) = 0;
    protected:
        uint8_t _numPRGBanks{0};
        uint8_t _numCHRBanks{0};    
    };
} // namespace NES
#endif // NES_MAPPER_H