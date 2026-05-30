#ifndef NES_CARTRIDGE_H
#define NES_CARTRIDGE_H

#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include "nes_mapper.h"
#include "mappers/nes_mapper000.h"

namespace NES
{
    class Cartridge
    {
    public:
        Cartridge(const std::string &filename);

        Cartridge(const Cartridge &) = delete;
        Cartridge &operator=(const Cartridge &) = delete;
        Cartridge(Cartridge &&) = delete;
        Cartridge &operator=(Cartridge &&) = delete;

        std::optional<uint8_t> cpuRead(uint16_t address) const;
        void cpuWrite(uint16_t address, uint8_t value);
        std::optional<uint8_t> ppuRead(uint16_t address) const;
        void ppuWrite(uint16_t address, uint8_t value);

    private:
        std::vector<uint8_t> _prgROM;
        std::vector<uint8_t> _chrROM;
        std::unique_ptr<Mapper> _mapper;

        uint8_t _mapperID{0};
        uint8_t _numPRGBanks{0};
        uint8_t _numCHRBanks{0};

        struct Header
        {
            uint8_t signature[4];
            uint8_t prgROMSize;
            uint8_t chrROMSize;
            uint8_t flags6;
            uint8_t flags7;
            uint8_t prgRAMSize;
            uint8_t flags9;
            uint8_t flags10;
            uint8_t zero[5];
        } _header;

        uint8_t getFileFormatVersion() const;

        static std::unique_ptr<Mapper> createMapper(uint8_t mapperID, uint8_t numPRGBanks, uint8_t numCHRBanks);
    };
} // namespace NES
#endif // NES_CARTRIDGE_H