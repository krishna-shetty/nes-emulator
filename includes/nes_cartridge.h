#ifndef NES_CARTRIDGE_H
#define NES_CARTRIDGE_H

#include <cstdint>
#include <vector>
#include <string>

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

            uint8_t read(uint16_t address) const;
            void write(uint16_t address, uint8_t value);
        private:
            std::vector<uint8_t> _prgROM;
            std::vector<uint8_t> _chrROM;

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
    };
} // namespace NES
# endif // NES_CARTRIDGE_H