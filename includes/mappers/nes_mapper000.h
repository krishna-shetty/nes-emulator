#ifndef NES_MAPPER000_H
#define NES_MAPPER000_H

#include "nes_mapper.h"

namespace NES
{
    class Mapper000 : public Mapper
    {
    public:
        Mapper000(uint8_t numPRGBanks, uint8_t numCHRBanks);
        ~Mapper000() = default;

        std::optional<uint32_t> cpuRead(uint16_t address) override;
        std::optional<uint32_t> cpuWrite(uint16_t address) override;
        std::optional<uint32_t> ppuRead(uint16_t address) override;
        std::optional<uint32_t> ppuWrite(uint16_t address) override;
    };
} // namespace NES
#endif // NES_MAPPER000_H