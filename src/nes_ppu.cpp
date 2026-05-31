#include "nes_ppu.h"
#include <stdexcept>
#include "nes.h"

using namespace NES;

PPU::PPU(const char *title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
        throw std::runtime_error(SDL_GetError());

    createWindow(title, width, height);
}

PPU::~PPU() noexcept
{
    destroyWindow();
    SDL_Quit();
}

void PPU::createWindow(const char *title, int width, int height)
{
    if (!SDL_CreateWindowAndRenderer(title, width, height, 0, &_window, &_renderer))
    {
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }
}

void PPU::destroyWindow() noexcept
{
    if (_renderer)
    {
        SDL_DestroyRenderer(_renderer);
        _renderer = nullptr;
    }
    if (_window)
    {
        SDL_DestroyWindow(_window);
        _window = nullptr;
    }
}

void PPU::handleEvents(std::function<void(SDL_Event &)> callback)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (callback)
            callback(event);
        if (event.type == SDL_EVENT_QUIT)
            _running = false;
    }
}

void PPU::clear()
{
    SDL_SetRenderDrawColor(_renderer, _clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    SDL_RenderClear(_renderer);
}

void PPU::present()
{
    SDL_RenderPresent(_renderer);
}

void PPU::setClearColor(SDL_Color color)
{
    _clearColor = color;
}

void PPU::clock()
{
    if (_scanline == 241 && _cycle == 1)
    {
        setFlag(Status::V, true); // Set VBlank flag
    }

    _cycle++;

    if (_cycle > 340)
    {
        _cycle = 0;
        _scanline++;
        if (_scanline >= 261)
        {
            _scanline = -1;
            _frameComplete = true;
        }
    }
}

void PPU::insertCartridge(std::shared_ptr<Cartridge> cartridge)
{
    _cartridge = cartridge;
}

uint8_t PPU::ppuRead(uint16_t address)
{
    uint8_t data = 0x00;
    address &= 0x3FFF;

    if (address <= 0x1FFF) // CHR-ROM
    {
        auto mapped = _cartridge->ppuRead(address);

        if (mapped)
            data = *mapped;
    }
    else if (address <= 0x3EFF) // nametables
    {
        // _tableName reads
    }
    else // $3F00-$3FFF palette
    {
        address &= 0x001F;
        if (address == 0x0010)
            address = 0x0000;
        else if (address == 0x0014)
            address = 0x0004;
        else if (address == 0x0018)
            address = 0x0008;
        else if (address == 0x001C)
            address = 0x000C;
        data = _tablePalette[address];
    }

    return data;
}

void PPU::ppuWrite(uint16_t address, uint8_t value)
{
    address &= 0x3FFF;

    if (address <= 0x1FFF) // CHR-ROM
    {
        _cartridge->ppuWrite(address, value);
    }
    else if (address <= 0x3EFF) // nametables
    {
        // _tableName writes
    }
    else // $3F00-$3FFF palette
    {
        address &= 0x001F;
        if (address == 0x0010)
            address = 0x0000;
        else if (address == 0x0014)
            address = 0x0004;
        else if (address == 0x0018)
            address = 0x0008;
        else if (address == 0x001C)
            address = 0x000C;
        _tablePalette[address] = value;
    }
}

void PPU::cpuWrite(uint16_t address, uint8_t value)
{
    switch (address)
    {
    case 0x0000: // Control
        _control = value;
        break;
    case 0x0001: // Mask
        _mask = value;
        break;
    case 0x0002: // Status
        break;   // Read-only
    case 0x0003: // OAM Address
        break;   // Not implemented
    case 0x0004: // OAM Data
        break;   // Not implemented
    case 0x0005: // Scroll
        break;   // Not implemented
    case 0x0006: // PPU Address
        if (_addressLatch == 0)
        {
            _vramAddress = (_vramAddress & 0x00FF) | ((uint16_t)value << 8); // High byte
            _addressLatch = 1;
        }
        else
        {
            // Second write (low byte)
            _vramAddress = (_vramAddress & 0xFF00) | value; // Low byte
            _addressLatch = 0;
        }
        break;
    case 0x0007: // PPU Data
        ppuWrite(_vramAddress, value);
        _vramAddress++;  
        break;
    }
}

uint8_t PPU::cpuRead(uint16_t address)
{
    uint8_t data = 0x00;
    switch (address)
    {
    case 0x0000: // Control
        break;
    case 0x0001: // Mask
        break;
    case 0x0002: // Status
        data = (_status & 0xE0) | (_ppuDataBuffer & 0x1F); // Return status and lower bits of last read
        setFlag(Status::V, false); // Clear VBlank flag
        _addressLatch = 0; // Reset address latch
        break;
    case 0x0003: // OAM Address
        break;   // Not implemented
    case 0x0004: // OAM Data
        break;   // Not implemented
    case 0x0005: // Scroll
        break;   // Not implemented
    case 0x0006: // PPU Address
        break;   // Not implemented
    case 0x0007: // PPU Data
        data = _ppuDataBuffer;
        _ppuDataBuffer = ppuRead(_vramAddress);

        if (_vramAddress >= 0x3F00)
            data = _ppuDataBuffer; // No delay for palette reads

        _vramAddress++;
    }
    return data;
}

void PPU::setFlag(Control flag, bool value)
{
    if (value)
        NES::setBit(_control, static_cast<uint8_t>(flag));
    else
        NES::clearBit(_control, static_cast<uint8_t>(flag));
}

void PPU::setFlag(Mask flag, bool value)
{
    if (value)
        NES::setBit(_mask, static_cast<uint8_t>(flag));
    else
        NES::clearBit(_mask, static_cast<uint8_t>(flag));
}

void PPU::setFlag(Status flag, bool value)
{
    if (value)
        NES::setBit(_status, static_cast<uint8_t>(flag));
    else
        NES::clearBit(_status, static_cast<uint8_t>(flag));
}

uint8_t PPU::getFlag(Status flag) const
{
    return NES::getBit(_status, static_cast<uint8_t>(flag));
}

uint8_t PPU::getFlag(Control flag) const
{
    return NES::getBit(_control, static_cast<uint8_t>(flag));
}

uint8_t PPU::getFlag(Mask flag) const
{
    return NES::getBit(_mask, static_cast<uint8_t>(flag));
}