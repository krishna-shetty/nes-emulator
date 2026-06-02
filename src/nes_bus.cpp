#include "nes_bus.h"
#include "nes_not_implemented.h"

using namespace NES;

uint8_t Bus::read(uint16_t address)
{
    uint8_t data = _openBus;

    // $0000-$1FFF : 2KB internal RAM, mirrored every $0800
    if (address < 0x2000)
    {
        data = _ram.read(address & 0x07FF);
    }
    // $2000-$3FFF : PPU registers, mirrored every 8 bytes
    else if (address >= 0x2000 && address < 0x4000)
    {
        if (_ppu)
        {
            data = _ppu->cpuRead(address & 0x0007);
        }
    }
    // $4000-$4017 : APU and I/O registers
    else if (address >= 0x4000 && address < 0x4018)
    {
        if (address == 0x4016)
        {
            data = _controller1.read();
        }
        else if (address == 0x4017)
        {
            data = _controller2.read();
        }
    }
    // $4018-$401F : APU and I/O functionality that is normally disabled
    else if (address >= 0x4018 && address < 0x4020)
    {
        // TODO: Implement APU and I/O functionality that is normally disabled
    }
    // $4020-$FFFF : cartridge use
    else
    {
        if (_cartridge)
        {
            auto cartridgeData = _cartridge->cpuRead(address);
            if (cartridgeData)
                data = *cartridgeData;
        }
    }

    _openBus = data;
    return data;
}

void Bus::write(uint16_t address, uint8_t value)
{
    // $0000-$1FFF : 2KB internal RAM, mirrored every $0800
    if (address < 0x2000)
    {
        _ram.write(address & 0x07FF, value);
    }
    // $2000-$3FFF : PPU registers, mirrored every 8 bytes
    else if (address >= 0x2000 && address < 0x4000)
    {
        if (_ppu)
        {
            _ppu->cpuWrite(address & 0x0007, value);
        }
    }
    // $4000-$4017 : APU and I/O registers
    else if (address >= 0x4000 && address < 0x4018)
    {
        // TODO: Implement APU and I/O registers
        if (address == 0x4014) // OAM DMA
        {
            _dmaPage = value;
            _dmaAddress = 0x00;
            _dmaInProgress = true;
        }
        else if (address == 0x4016)
        {
            _controller1.strobe(value);
        }
        else if (address == 0x4017)
        {
            _controller2.strobe(value);
        }
    }
    // $4018-$401F : APU and I/O functionality that is normally disabled
    else if (address >= 0x4018 && address < 0x4020)
    {
        // TODO: Implement APU and I/O functionality that is normally disabled
        // throw NotImplemented("TODO: See CPU Test Mode on nesdev.org", __func__);
    }
    // $4020-$FFFF : cartridge use
    else
    {
        if (_cartridge)
        {
            _cartridge->cpuWrite(address, value);
        }
    }
}

void Bus::loadProgram(uint16_t address, std::vector<uint8_t> const &bytes)
{
    for (size_t i = 0; i < bytes.size(); i++)
    {
        _ram.write((address + i) & 0x07FF, bytes[i]);
    }
}

uint8_t Bus::peek(uint16_t address) const
{
    if (address < 0x2000)
    {
        return _ram.read(address & 0x07FF);
    }

    if (address >= 0x2000 && address < 0x4000)
    {
        if (_ppu)
            return _ppu->cpuPeek(address & 0x0007);
    }

    if (address >= 0x8000) // cartridge ROM
    {
        if (_cartridge)
        {
            auto data = _cartridge->cpuRead(address);
            if (data)
                return *data;
        }
    }

    return 0;
}

void Bus::insertCartridge(std::shared_ptr<Cartridge> cartridge)
{
    _cartridge = cartridge;
}

void Bus::connectPPU(PPU *ppu)
{
    _ppu = ppu;
}

Controller &Bus::getController1()
{
    return _controller1;
}

Controller &Bus::getController2()
{
    return _controller2;
}

bool Bus::getDMAInProgress() const
{
    return _dmaInProgress;
}

bool Bus::getDMADummyCycle() const
{
    return _dmaDummyCycle;
}

void Bus::setDMAInProgress(bool inProgress)
{
    _dmaInProgress = inProgress;
}

void Bus::setDMADummyCycle(bool dummyCycle)
{
    _dmaDummyCycle = dummyCycle;
}

void Bus::setDMAData(uint8_t data)
{
    _dmaData = data;
}

void Bus::incrementDMAAddress(uint8_t increment)
{
    _dmaAddress += increment;
}

uint8_t Bus::getDMAAddress() const
{
    return _dmaAddress;
}

uint8_t Bus::getDMAData() const
{
    return _dmaData;
}

uint8_t Bus::getDMAPage() const
{
    return _dmaPage;
}