#include "nes_cartridge.h"
#include <fstream>
#include <stdexcept>

using namespace NES;

Cartridge::Cartridge(const std::string &filename)
{
    std::ifstream file;
    file.open(filename, std::ios::binary);

    if (!file)
    {
        throw std::runtime_error("Failed to open cartridge file: " + filename);
    }

    if(file.is_open())
    {
        file.read(reinterpret_cast<char*>(&_header), sizeof(Header));

        if (_header.signature[0] != 'N' || _header.signature[1] != 'E' || _header.signature[2] != 'S' || _header.signature[3] != 0x1A)
        {
            throw std::runtime_error("Invalid NES file format: " + filename);
        }

        if(_header.flags6 & 0x04)
        {
            file.seekg(512, std::ios::cur);
        }

        // Determine mapper ID
        _mapperID = (_header.flags7 >> 4) << 4 | (_header.flags6 >> 4);

        const uint8_t fileFormatVersion = getFileFormatVersion();

        // Currently, we only support the standard iNES format (version 1)
        if (fileFormatVersion == 1)
        {
            _numPRGBanks = _header.prgROMSize;
            _numCHRBanks = _header.chrROMSize;

            _prgROM.resize(_numPRGBanks * 16384);
            file.read(reinterpret_cast<char*>(_prgROM.data()), _prgROM.size());

            _chrROM.resize(_numCHRBanks * 8192);
            file.read(reinterpret_cast<char*>(_chrROM.data()), _chrROM.size());
        }
        
        _mapper = createMapper(_mapperID, _numPRGBanks, _numCHRBanks);
    }
}
    
uint8_t Cartridge::getFileFormatVersion() const
{
    if((_header.flags7 & 0x0C) == 0x04)
    {
        return 0; // Archaic iNES format
    }

    if((_header.flags7 & 0x0C) == 0x00)
    {
        return 1; // Standard iNES format
    }

    if((_header.flags7 & 0x0C) == 0x08)
    {
        return 2; // NES 2.0 format
    }

    throw std::runtime_error("Unsupported file format");
}

std::unique_ptr<Mapper> Cartridge::createMapper(uint8_t mapperID, uint8_t numPRGBanks, uint8_t numCHRBanks)
{
    switch (mapperID)
    {
        case 0:
            return std::make_unique<Mapper000>(numPRGBanks, numCHRBanks);
        default:
            throw std::runtime_error("Unsupported mapper ID: " + std::to_string(mapperID));
    }
}

std::optional<uint8_t> Cartridge::cpuRead(uint16_t address) const
{
    if (_mapper)
    {
        std::optional<uint32_t> addr = _mapper->cpuRead(address);

        if(addr.has_value())
        {
            return _prgROM[addr.value()];
        }
    }
    return std::nullopt;
}

void Cartridge::cpuWrite(uint16_t address, uint8_t value)
{
    if (_mapper)
    {
        std::optional<uint32_t> addr = _mapper->cpuWrite(address);

        if (addr.has_value())
        {
            _prgROM[addr.value()] = value;
        }
    }
}

std::optional<uint8_t> Cartridge::ppuRead(uint16_t address) const
{
    if (_mapper)
    {
        std::optional<uint32_t> addr = _mapper->ppuRead(address);

        if(addr.has_value())
        {
            return _chrROM[addr.value()];
        }
    }
    return std::nullopt;
}

void Cartridge::ppuWrite(uint16_t address, uint8_t value)
{
    if (_mapper)
    {
        std::optional<uint32_t> addr = _mapper->ppuWrite(address);

        if (addr.has_value())
        {
            _chrROM[addr.value()] = value;
        }
    }
}
