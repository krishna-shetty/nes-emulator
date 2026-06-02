#include "nes_ppu.h"
#include <stdexcept>
#include "nes_utils.h"
#include <iostream>
#include <algorithm>

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
    SDL_SetRenderVSync(_renderer, 1);
    _screenTexture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_ABGR8888,
                                       SDL_TEXTUREACCESS_STREAMING, 256, 240);
    SDL_SetTextureScaleMode(_screenTexture, SDL_SCALEMODE_NEAREST);
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
    if (_screenTexture)
    {
        SDL_DestroyTexture(_screenTexture);
        _screenTexture = nullptr;
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

void PPU::draw()
{
    SDL_UpdateTexture(_screenTexture, nullptr, _screenBuffer, 256 * sizeof(SDL_Color));
    SDL_RenderTexture(_renderer, _screenTexture, nullptr, nullptr);
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

    if (_scanline >= -1 && _scanline < 240)
    {
        // ============================================================
        // BACKGROUND RENDERING
        // ============================================================

        if (_scanline == -1 && _cycle == 1)
        {
            setFlag(Status::V, false); // Clear VBlank flag
            _frameComplete = false;

            setFlag(Status::S, false); // Clear Sprite 0 Hit flag
            setFlag(Status::O, false); // Clear Sprite Overflow flags
            for (int i = 0; i < 8; i++)
            {
                _spriteShifterPatternLo[i] = 0;
                _spriteShifterPatternHi[i] = 0;
            }
        }
        if ((_cycle >= 2 && _cycle < 258) || (_cycle >= 321 && _cycle < 338))
        {
            updateShifters();
            switch ((_cycle - 1) % 8)
            {
            case 0:
                loadBackgroundShifters();
                // Fetch the next background tile ID
                _backgroundNextTileID = ppuRead(0x2000 | (_vramAddress.reg & 0x0FFF));
                break;
            case 2:
                // Fetch the next background tile attribute
                _backgroundNextTileAttrib = ppuRead(0x23C0 | (_vramAddress.nametableY << 11) | (_vramAddress.nametableX << 10) | ((_vramAddress.coarseY >> 2) << 3) | (_vramAddress.coarseX >> 2));
                if (_vramAddress.coarseY & 0x02)
                {
                    _backgroundNextTileAttrib >>= 4;
                }
                if (_vramAddress.coarseX & 0x02)
                {
                    _backgroundNextTileAttrib >>= 2;
                }
                _backgroundNextTileAttrib &= 0x03;
                break;
            case 4:
                _backgroundNextTileLSB = ppuRead(
                    (static_cast<uint16_t>(getFlag(Control::B)) << 12) |
                    (static_cast<uint16_t>(_backgroundNextTileID) << 4) | // Multiply by 16
                    _vramAddress.fineY);
                break;
            case 6:
                _backgroundNextTileMSB = ppuRead(
                    (static_cast<uint16_t>(getFlag(Control::B)) << 12) |
                    (static_cast<uint16_t>(_backgroundNextTileID) << 4) | // Multiply by 16
                    _vramAddress.fineY | 0x0008);
                break;
            case 7:
                incrementScrollX();
                break;
            }
        }
        if (_cycle == 256)
        {
            incrementScrollY();
        }

        if (_cycle == 257)
        {
            transferAddressX();
        }

        if (_scanline == -1 && _cycle >= 280 && _cycle < 305)
        {
            transferAddressY();
        }

        if (_cycle == 338 || _cycle == 340)
        {
            _backgroundNextTileID = ppuRead(0x2000 | (_vramAddress.reg & 0x0FFF));
        }

        // ============================================================
        // FOREGROUND RENDERING
        // ============================================================

        if (_cycle == 257 && _scanline >= 0)
        {
            std::fill(_spriteScanline.begin(),
                      _spriteScanline.end(),
                      Sprite{255, 255, 255, 255});
            _spriteCount = 0;

            uint8_t numOAMEntries = 0;
            _spriteZeroHitPossible = false;
            while (numOAMEntries < 64 && _spriteCount < 9)
            {
                int16_t diff = static_cast<int16_t>(_scanline) - static_cast<int16_t>(_OAM[numOAMEntries].y);
                if (diff >= 0 && diff < (getFlag(Control::H) ? 16 : 8))
                {
                    if (_spriteCount < 8)
                    {
                        if (numOAMEntries == 0)
                        {
                            _spriteZeroHitPossible = true;
                        }
                        _spriteScanline[_spriteCount] = _OAM[numOAMEntries];
                    }
                    _spriteCount++;
                }
                numOAMEntries++;
            }
            if (_spriteCount > 8)
            {
                _spriteCount = 8;
                setFlag(Status::O, true); // Set sprite overflow flag
            }
        }

        if (_cycle == 340)
        {
            for (uint8_t i = 0; i < _spriteCount; i++)
            {
                uint16_t spritePatternAddrLo{0};
                uint16_t spritePatternAddrHi{0};

                uint8_t spritePatternBitsLo{0};
                uint8_t spritePatternBitsHi{0};

                if (!getFlag(Control::H)) // 8x8 sprites
                {
                    if (!getFlag(Control::H)) // 8x8 sprites
                    {
                        if (!(_spriteScanline[i].attributes & 0x80)) // no vertical flip
                        {
                            spritePatternAddrLo = (getFlag(Control::S) << 12) |
                                                  (_spriteScanline[i].tileIndex << 4) |
                                                  (static_cast<uint16_t>(_scanline) - _spriteScanline[i].y);
                        }
                        else // vertical flip
                        {
                            spritePatternAddrLo = (getFlag(Control::S) << 12) |
                                                  (_spriteScanline[i].tileIndex << 4) |
                                                  (7 - (static_cast<uint16_t>(_scanline) - _spriteScanline[i].y));
                        }
                    }
                    else
                    {
                        // 8x16 sprites
                        if (!(_spriteScanline[i].attributes & 0x80)) // No vertical flip
                        {
                            // No vertical flip
                            if (static_cast<uint16_t>(_scanline) - _spriteScanline[i].y < 8)
                            {
                                spritePatternAddrLo = ((static_cast<uint16_t>(_spriteScanline[i].tileIndex) & 0x01) << 12) | ((static_cast<uint16_t>(_spriteScanline[i].tileIndex) & 0xFE) << 4) | ((static_cast<uint16_t>(_scanline) - _spriteScanline[i].y) & 0x07);
                            }
                            else
                            {
                                spritePatternAddrLo = ((static_cast<uint16_t>(_spriteScanline[i].tileIndex) & 0x01) << 12) | ((static_cast<uint16_t>((_spriteScanline[i].tileIndex) & 0xFE) + 1) << 4) | ((static_cast<uint16_t>(_scanline) - _spriteScanline[i].y) & 0x07);
                            }
                        }
                        else // Vertical flip
                        {
                            if (static_cast<uint16_t>(_scanline) - _spriteScanline[i].y < 8)
                            {
                                spritePatternAddrLo = ((static_cast<uint16_t>(_spriteScanline[i].tileIndex) & 0x01) << 12) | ((static_cast<uint16_t>((_spriteScanline[i].tileIndex) & 0xFE) + 1) << 4) | (7 - (static_cast<uint16_t>(_scanline) - _spriteScanline[i].y));
                            }
                            else
                            {
                                spritePatternAddrLo = ((static_cast<uint16_t>(_spriteScanline[i].tileIndex) & 0x01) << 12) | ((static_cast<uint16_t>(_spriteScanline[i].tileIndex) & 0xFE) << 4) | (7 - ((static_cast<uint16_t>(_scanline) - _spriteScanline[i].y) & 0x07));
                            }
                        }
                    }
                }
                spritePatternAddrHi = spritePatternAddrLo + 8;

                spritePatternBitsLo = ppuRead(spritePatternAddrLo);
                spritePatternBitsHi = ppuRead(spritePatternAddrHi);

                if (_spriteScanline[i].attributes & 0x40) // Horizontal flip
                {
                    spritePatternBitsLo = reverseByte(spritePatternBitsLo);
                    spritePatternBitsHi = reverseByte(spritePatternBitsHi);
                }

                _spriteShifterPatternLo[i] = spritePatternBitsLo;
                _spriteShifterPatternHi[i] = spritePatternBitsHi;
            }
        }
    }

    if (_scanline == 240)
    {
        // Do nothing, post-render scanline
    }

    if (_scanline == 241 && _cycle == 1)
    {
        setFlag(Status::V, true); // Set VBlank flag

        if (getFlag(Control::V))
        {
            _nmiRequest = true; // Trigger NMI
        }
    }

    uint8_t backgroundPixel = 0x00;
    uint8_t backgroundPalette = 0x00;

    if (getFlag(Mask::b))
    {
        uint16_t bitMux = 0x8000 >> _fineX;

        uint8_t p0Pixel = (backgroundShifterPatternLo & bitMux) > 0; // LSB plane bit
        uint8_t p1Pixel = (backgroundShifterPatternHi & bitMux) > 0; // MSB plane bit
        backgroundPixel = (p1Pixel << 1) | p0Pixel;

        uint8_t bgPal0 = (backgroundShifterAttribLo & bitMux) > 0;
        uint8_t bgPal1 = (backgroundShifterAttribHi & bitMux) > 0;
        backgroundPalette = (bgPal1 << 1) | bgPal0;
    }

    uint8_t foregroundPixel = 0x00;
    uint8_t foregroundPalette = 0x00;
    uint8_t foregroundPriority = 0x00;

    if (getFlag(Mask::s))
    {
        _isSpriteZeroBeingRendered = false;

        for (uint8_t i = 0; i < _spriteCount; i++)
        {
            if (_spriteScanline[i].x == 0)
            {
                uint8_t spritePixelLo = (_spriteShifterPatternLo[i] & 0x80) > 0;
                uint8_t spritePixelHi = (_spriteShifterPatternHi[i] & 0x80) > 0;
                foregroundPixel = (spritePixelHi << 1) | spritePixelLo;

                foregroundPalette = (_spriteScanline[i].attributes & 0x03) + 0x04; // Sprite palettes start at index 4
                foregroundPriority = (_spriteScanline[i].attributes & 0x20) == 0;  // Priority bit

                if (foregroundPixel != 0)
                {
                    if (i == 0)
                    {
                        _isSpriteZeroBeingRendered = true;
                    }
                    break; // Found a non-transparent sprite pixel
                }
            }
        }
    }

    uint8_t finalPixel = 0x00;
    uint8_t finalPalette = 0x00;

    if (backgroundPixel == 0 && foregroundPixel == 0)
    {
        finalPixel = 0;
        finalPalette = 0;
    }
    else if (backgroundPixel == 0 && foregroundPixel != 0)
    {
        finalPixel = foregroundPixel;
        finalPalette = foregroundPalette;
    }
    else if (backgroundPixel != 0 && foregroundPixel == 0)
    {
        finalPixel = backgroundPixel;
        finalPalette = backgroundPalette;
    }
    else
    {
        if (foregroundPriority)
        {
            finalPixel = foregroundPixel;
            finalPalette = foregroundPalette;
        }
        else
        {
            finalPixel = backgroundPixel;
            finalPalette = backgroundPalette;
        }

        if (_spriteZeroHitPossible && _isSpriteZeroBeingRendered)
        {
            if (getFlag(Mask::b) && getFlag(Mask::s))
            {
                if (!((backgroundPixel & 0x01) == 0 && (foregroundPixel & 0x01) == 0))
                {
                    if (_cycle >= 9 && _cycle < 258)
                    {
                        setFlag(Status::S, true); // Set sprite zero hit flag
                    }
                }
                else
                {
                    if (_cycle >= 1 && _cycle < 258)
                    {
                        setFlag(Status::S, true); // Set sprite zero hit flag
                    }
                }
            }
        }
    }

    if (_cycle >= 1 && _cycle <= 256 && _scanline >= 0 && _scanline < 240)
    {
        if (_cycle >= 1 && _cycle <= 256 && _scanline >= 0 && _scanline < 240)
        {
            uint8_t colorIndex;

            if (finalPixel == 0)
            {
                colorIndex = _tablePalette[0x00] & 0x3F;
            }
            else
            {
                uint8_t paletteAddr =
                    ((finalPalette << 2) | finalPixel) & 0x1F;

                colorIndex = _tablePalette[paletteAddr] & 0x3F;
            }

            SDL_Color color = PALETTE[colorIndex];

            _screenBuffer[_scanline * 256 + (_cycle - 1)] = color;
        }
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
    else if (address >= 0x2000 && address <= 0x3EFF) // nametables
    {
        if (_cartridge)
        {
            NES::Cartridge::Mirroring mirroring = _cartridge->getMirroring();
            uint16_t offset = address & 0x0FFF;
            if (mirroring == NES::Cartridge::Mirroring::VERTICAL)
            {
                // Vertical mirroring:
                if (offset >= 0x0000 && offset <= 0x03FF)
                    data = _tableName[0][address & 0x03FF];
                else if (offset >= 0x0400 && offset <= 0x07FF)
                    data = _tableName[1][address & 0x03FF];
                else if (offset >= 0x0800 && offset <= 0x0BFF)
                    data = _tableName[0][address & 0x03FF];
                else if (offset >= 0x0C00 && offset <= 0x0FFF)
                    data = _tableName[1][address & 0x03FF];
            }
            else if (mirroring == NES::Cartridge::Mirroring::HORIZONTAL)
            {
                // Horizontal mirroring:
                if (offset >= 0x0000 && offset <= 0x03FF)
                    data = _tableName[0][address & 0x03FF];
                else if (offset >= 0x0400 && offset <= 0x07FF)
                    data = _tableName[0][address & 0x03FF];
                else if (offset >= 0x0800 && offset <= 0x0BFF)
                    data = _tableName[1][address & 0x03FF];
                else if (offset >= 0x0C00 && offset <= 0x0FFF)
                    data = _tableName[1][address & 0x03FF];
            }
        }
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
    else if (address >= 0x2000 && address <= 0x3EFF) // nametables
    {
        if (_cartridge)
        {
            NES::Cartridge::Mirroring mirroring = _cartridge->getMirroring();
            uint16_t offset = address & 0x0FFF;
            if (mirroring == NES::Cartridge::Mirroring::VERTICAL)
            {
                // Vertical mirroring:
                if (offset >= 0x0000 && offset <= 0x03FF)
                    _tableName[0][address & 0x03FF] = value;
                else if (offset >= 0x0400 && offset <= 0x07FF)
                    _tableName[1][address & 0x03FF] = value;
                else if (offset >= 0x0800 && offset <= 0x0BFF)
                    _tableName[0][address & 0x03FF] = value;
                else if (offset >= 0x0C00 && offset <= 0x0FFF)
                    _tableName[1][address & 0x03FF] = value;
            }
            else if (mirroring == NES::Cartridge::Mirroring::HORIZONTAL)
            {
                // Horizontal mirroring:
                if (offset >= 0x0000 && offset <= 0x03FF)
                    _tableName[0][address & 0x03FF] = value;
                else if (offset >= 0x0400 && offset <= 0x07FF)
                    _tableName[0][address & 0x03FF] = value;
                else if (offset >= 0x0800 && offset <= 0x0BFF)
                    _tableName[1][address & 0x03FF] = value;
                else if (offset >= 0x0C00 && offset <= 0x0FFF)
                    _tableName[1][address & 0x03FF] = value;
            }
        }
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

        _tablePalette[address] = value;
    }
}

void PPU::cpuWrite(uint16_t address, uint8_t value)
{
    switch (address)
    {
    case 0x0000: // Control
        _control = value;
        _tramAddress.nametableX = getFlag(Control::Nx);
        _tramAddress.nametableY = getFlag(Control::Ny);
        break;
    case 0x0001: // Mask
        _mask = value;
        break;
    case 0x0002: // Status
        break;   // Read-only
    case 0x0003: // OAM Address
        _OAMAddress = value;
        break;   // Not implemented
    case 0x0004: // OAM Data
        _OAMPtr[_OAMAddress] = value;
        _OAMAddress++;
        break;   // Not implemented
    case 0x0005: // Scroll
        if (_addressLatch == 0)
        {
            _fineX = value & 0x07;             // Fine X scroll
            _tramAddress.coarseX = value >> 3; // Coarse X scroll
            _addressLatch = 1;
        }
        else
        {
            _tramAddress.fineY = value & 0x07; // Fine Y scroll
            _tramAddress.coarseY = value >> 3; // Coarse Y scroll
            _addressLatch = 0;
        }
        break;
    case 0x0006: // PPU Address
        if (_addressLatch == 0)
        {
            _tramAddress.reg = (_tramAddress.reg & 0x00FF) | ((uint16_t)(value & 0x3F) << 8); // High byte
            _addressLatch = 1;
        }
        else
        {
            // Second write (low byte)
            _tramAddress.reg = (_tramAddress.reg & 0xFF00) | value; // Low byte
            _vramAddress.reg = _tramAddress.reg;                    // Update VRAM address
            _addressLatch = 0;
        }
        break;
    case 0x0007: // PPU Data
        ppuWrite(_vramAddress.reg, value);
        _vramAddress.reg += (getFlag(Control::I) ? 32 : 1); // Increment by 1 or 32 based on Control flag
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
    case 0x0002:                                           // Status
        data = (_status & 0xE0) | (_ppuDataBuffer & 0x1F); // Return status and lower bits of last read
        setFlag(Status::V, false);                         // Clear VBlank flag
        _addressLatch = 0;                                 // Reset address latch
        break;
    case 0x0003: // OAM Address
        break;   // Not implemented
    case 0x0004: // OAM Data
        data = _OAMPtr[_OAMAddress];
        break;   // Not implemented
    case 0x0005: // Scroll
        break;   // Not implemented
    case 0x0006: // PPU Address
        break;   // Not implemented
    case 0x0007: // PPU Data
        data = _ppuDataBuffer;
        _ppuDataBuffer = ppuRead(_vramAddress.reg);

        if (_vramAddress.reg >= 0x3F00)
            data = _ppuDataBuffer; // No delay for palette reads

        _vramAddress.reg += (getFlag(Control::I) ? 32 : 1); // Increment by 1 or 32 based on Control flag
        break;
    }
    return data;
}

uint8_t PPU::cpuPeek(uint16_t address) const
{
    uint8_t data = 0x00;
    switch (address)
    {
    case 0x0002:                                           // Status
        data = (_status & 0xE0) | (_ppuDataBuffer & 0x1F); // Return status and lower bits of last read
        break;
    case 0x0007: // PPU Data
        data = _ppuDataBuffer;
        break;
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

void PPU::incrementScrollX()
{
    if (getFlag(Mask::b) || getFlag(Mask::s))
    {
        if (_vramAddress.coarseX == 31)
        {
            _vramAddress.coarseX = 0;
            _vramAddress.nametableX ^= 1; // Switch horizontal nametable
        }
        else
        {
            _vramAddress.coarseX++;
        }
    }
}

void PPU::incrementScrollY()
{
    if (getFlag(Mask::b) || getFlag(Mask::s))
    {
        if (_vramAddress.fineY < 7)
        {
            _vramAddress.fineY++;
        }
        else
        {
            _vramAddress.fineY = 0;
            if (_vramAddress.coarseY == 29)
            {
                _vramAddress.coarseY = 0;
                _vramAddress.nametableY ^= 1; // Switch vertical nametable
            }
            else if (_vramAddress.coarseY == 31)
            {
                _vramAddress.coarseY = 0; // Coarse Y wraps around without switching nametables
            }
            else
            {
                _vramAddress.coarseY++;
            }
        }
    }
}

void PPU::transferAddressX()
{
    if (getFlag(Mask::b) || getFlag(Mask::s))
    {
        _vramAddress.nametableX = _tramAddress.nametableX;
        _vramAddress.coarseX = _tramAddress.coarseX;
    }
}

void PPU::transferAddressY()
{
    if (getFlag(Mask::b) || getFlag(Mask::s))
    {
        _vramAddress.fineY = _tramAddress.fineY;
        _vramAddress.nametableY = _tramAddress.nametableY;
        _vramAddress.coarseY = _tramAddress.coarseY;
    }
}

void PPU::loadBackgroundShifters()
{
    backgroundShifterPatternLo = (backgroundShifterPatternLo & 0xFF00) | _backgroundNextTileLSB;
    backgroundShifterPatternHi = (backgroundShifterPatternHi & 0xFF00) | _backgroundNextTileMSB;

    backgroundShifterAttribLo = (backgroundShifterAttribLo & 0xFF00) | ((_backgroundNextTileAttrib & 0x01) ? 0xFF : 0x00);
    backgroundShifterAttribHi = (backgroundShifterAttribHi & 0xFF00) | ((_backgroundNextTileAttrib & 0x02) ? 0xFF : 0x00);
}

void PPU::updateShifters()
{
    if (getFlag(Mask::b))
    {
        backgroundShifterPatternLo <<= 1;
        backgroundShifterPatternHi <<= 1;
        backgroundShifterAttribLo <<= 1;
        backgroundShifterAttribHi <<= 1;
    }

    if (getFlag(Mask::s) && _cycle >= 1 && _cycle < 258)
    {
        for (uint8_t i = 0; i < _spriteCount; i++)
        {
            if (_spriteScanline[i].x > 0)
            {
                _spriteScanline[i].x--;
            }
            else
            {
                _spriteShifterPatternLo[i] <<= 1;
                _spriteShifterPatternHi[i] <<= 1;
            }
        }
    }
}

bool PPU::nmiRequested() const
{
    return _nmiRequest;
}

void PPU::clearNMI()
{
    _nmiRequest = false;
}

void PPU::setOAMData(uint8_t address, uint8_t value)
{
    _OAMPtr[address] = value;
}