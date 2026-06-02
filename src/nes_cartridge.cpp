#include "nes_cartridge.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

using namespace NES;

Cartridge::Cartridge(const std::string &filename)
{
    std::ifstream file;
    file.open(filename, std::ios::binary);

    if (!file)
    {
        throw std::runtime_error("Failed to open cartridge file: " + filename);
    }

    if (file.is_open())
    {
        file.read(reinterpret_cast<char *>(&_header), sizeof(Header));

        if (_header.signature[0] != 'N' || _header.signature[1] != 'E' || _header.signature[2] != 'S' || _header.signature[3] != 0x1A)
        {
            throw std::runtime_error("Invalid NES file format: " + filename);
        }

        if (_header.flags6 & 0x04)
        {
            file.seekg(512, std::ios::cur);
        }

        // Determine mapper ID
        _mapperID = (_header.flags6 >> 4) | (_header.flags7 & 0xF0); // after validating header
        if (_header.flags6 & 0x08)
            _mirroring = Mirroring::FOURSCREEN;
        else if (_header.flags6 & 0x01)
            _mirroring = Mirroring::VERTICAL;
        else
            _mirroring = Mirroring::HORIZONTAL;

        std::cout << "Signature: "
                  << _header.signature[0]
                  << _header.signature[1]
                  << _header.signature[2]
                  << " 0x" << std::hex << (int)_header.signature[3] << "\n";

        std::cout << "flags6: 0x" << std::hex << (int)_header.flags6 << "\n";
        std::cout << "flags7: 0x" << std::hex << (int)_header.flags7 << "\n";
        std::cout << "mapper: " << std::dec << (int)_mapperID << "\n";
        std::cout << "PRG banks: " << (int)_header.prgROMSize << "\n";
        std::cout << "CHR banks: " << (int)_header.chrROMSize << "\n";

        const uint8_t fileFormatVersion = getFileFormatVersion();

        // Currently, we only support the standard iNES format (version 1)
        // The archaic iNES format (version 0) is not supported.
        if (fileFormatVersion != 1)
        {
            throw std::runtime_error("Unsupported iNES format");
        }

        _numPRGBanks = _header.prgROMSize;
        _numCHRBanks = _header.chrROMSize;

        _prgROM.resize(_numPRGBanks * 16384);
        file.read(reinterpret_cast<char *>(_prgROM.data()), _prgROM.size());

        if (_numCHRBanks == 0)
        {
            _chrROM.resize(8192);
        }
        else
        {
            _chrROM.resize(_numCHRBanks * 8192);
            file.read(reinterpret_cast<char *>(_chrROM.data()), _chrROM.size());
        }

        _mapper = createMapper(_mapperID, _numPRGBanks, _numCHRBanks);
    }
}

uint8_t Cartridge::getFileFormatVersion() const
{
    if ((_header.flags7 & 0x0C) == 0x04)
    {
        return 0; // Archaic iNES format
    }

    if ((_header.flags7 & 0x0C) == 0x00)
    {
        return 1; // Standard iNES format
    }

    if ((_header.flags7 & 0x0C) == 0x08)
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

        if (addr.has_value())
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

        if (addr.has_value())
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

NES::Cartridge::Mirroring Cartridge::getMirroring() const
{
    return _mirroring;
}
