#ifndef NES_PPU_H
#define NES_PPU_H
#include <SDL3/SDL.h>
#include <functional>
#include "nes_cartridge.h"

namespace NES
{
    class PPU
    {
    public:
        enum class Status : uint8_t
        {
            V = 7, // Vertical blank
            S = 6, // Sprite 0 Hit
            O = 5, // Sprite Overflow
        };

        enum class Mask : uint8_t
        {
            G = 0,  // Greyscale
            m = 1,  // Background left column enable
            M = 2,  // Sprite left column enable
            b = 3,  // Background enable
            s = 4,  // Sprite enable
            R = 5,  // Emphasize red
            Gr = 6, // Emphasize green
            B = 7,  // Emphasize blue
        };

        enum class Control : uint8_t
        {
            Nx = 0, // Nametable select / X scroll bit
            Ny = 1, // Nametable select / Y scroll bit
            I = 2,  // Increment mode
            S = 3,  // Sprite tile select
            B = 4,  // Background tile select
            H = 5,  // Sprite height
            P = 6,  // PPU master/slave select
            V = 7,  // NMI enable
        };

        PPU(const char *title, int width, int height);
        ~PPU() noexcept;

        PPU(const PPU &) = delete;
        PPU &operator=(const PPU &) = delete;
        PPU(PPU &&) = delete;
        PPU &operator=(PPU &&) = delete;

        SDL_Window *getWindow() const { return _window; }
        SDL_Renderer *getRenderer() const { return _renderer; }

        bool isRunning() const { return _running; }
        void handleEvents(std::function<void(SDL_Event &)> callback = nullptr);
        void clear();
        void present();
        void clock();

        void setClearColor(SDL_Color color);
        uint8_t ppuRead(uint16_t address);
        void ppuWrite(uint16_t address, uint8_t value);
        uint8_t cpuRead(uint16_t address);
        void cpuWrite(uint16_t address, uint8_t value);

        static inline constexpr SDL_Color PALETTE[64] =
            {
                {84, 84, 84, 255},
                {0, 30, 116, 255},
                {8, 16, 144, 255},
                {48, 0, 136, 255},
                {68, 0, 100, 255},
                {92, 0, 48, 255},
                {84, 4, 0, 255},
                {60, 24, 0, 255},
                {32, 42, 0, 255},
                {8, 58, 0, 255},
                {0, 64, 0, 255},
                {0, 60, 0, 255},
                {0, 50, 60, 255},
                {0, 0, 0, 255},
                {0, 0, 0, 255},
                {0, 0, 0, 255},
                {152, 150, 152, 255},
                {8, 76, 196, 255},
                {48, 50, 236, 255},
                {92, 30, 228, 255},
                {136, 20, 176, 255},
                {160, 20, 100, 255},
                {152, 34, 32, 255},
                {120, 60, 0, 255},
                {84, 90, 0, 255},
                {40, 114, 0, 255},
                {8, 124, 0, 255},
                {0, 118, 40, 255},
                {0, 102, 120, 255},
                {0, 0, 0, 255},
                {0, 0, 0, 255},
                {0, 0, 0, 255},
                {236, 238, 236, 255},
                {76, 154, 236, 255},
                {120, 124, 236, 255},
                {176, 98, 236, 255},
                {228, 84, 236, 255},
                {236, 88, 180, 255},
                {236, 106, 100, 255},
                {212, 136, 32, 255},
                {160, 170, 0, 255},
                {116, 196, 0, 255},
                {76, 208, 32, 255},
                {56, 204, 108, 255},
                {56, 180, 204, 255},
                {60, 60, 60, 255},
                {0, 0, 0, 255},
                {0, 0, 0, 255},
                {236, 238, 236, 255},
                {168, 204, 236, 255},
                {188, 188, 236, 255},
                {212, 178, 236, 255},
                {236, 174, 236, 255},
                {236, 174, 212, 255},
                {236, 180, 176, 255},
                {228, 196, 144, 255},
                {204, 210, 120, 255},
                {180, 222, 120, 255},
                {168, 226, 144, 255},
                {152, 226, 180, 255},
                {160, 214, 228, 255},
                {160, 162, 160, 255},
                {0, 0, 0, 255},
                {0, 0, 0, 255},
        };

        void insertCartridge(std::shared_ptr<Cartridge> cartridge);

        void setFlag(Control flag, bool value);
        void setFlag(Mask flag, bool value);
        void setFlag(Status flag, bool value);
        uint8_t getFlag(Control flag) const;
        uint8_t getFlag(Mask flag) const;
        uint8_t getFlag(Status flag) const;

        uint8_t cpuPeek(uint16_t address) const;

        bool nmiRequested() const;
        void clearNMI();

    private:
        uint8_t _tableName[2][1024]; // 2KB of name tables
        uint8_t _tablePalette[32];

        int16_t _scanline{0};
        int16_t _cycle{0};

        bool _frameComplete{false};

        std::shared_ptr<Cartridge> _cartridge{nullptr};

        SDL_Window *_window{nullptr};
        SDL_Renderer *_renderer{nullptr};
        bool _running{true};

        SDL_Texture* _screenTexture{nullptr};
        SDL_Color _screenBuffer[256 * 240]; // NES resolution is 256x240
        SDL_Color _clearColor{0, 0, 0, 255};

        uint8_t _control{0};
        uint8_t _mask{0};
        uint8_t _status{0};

        uint8_t _addressLatch{0};
        uint8_t _ppuDataBuffer{0};

        // Shout out my homie Loopy (and javidx9)!
        union LoopyRegister
        {
            struct
            {
                uint16_t coarseX : 5;    // bits 0-4
                uint16_t coarseY : 5;    // bits 5-9
                uint16_t nametableX : 1; // bit 10
                uint16_t nametableY : 1; // bit 11
                uint16_t fineY : 3;      // bits 12-14
                uint16_t unused : 1;     // bit 15
            };
            uint16_t reg{0};
        };
        LoopyRegister _vramAddress{0};
        LoopyRegister _tramAddress{0};
        uint8_t _fineX{0};

        uint8_t _backgroundNextTileID{0};
        uint8_t _backgroundNextTileAttrib{0};
        uint8_t _backgroundNextTileLSB{0};
        uint8_t _backgroundNextTileMSB{0};

        uint16_t backgroundShifterPatternLo{0};
        uint16_t backgroundShifterPatternHi{0};
        uint16_t backgroundShifterAttribLo{0};
        uint16_t backgroundShifterAttribHi{0};

        bool _nmiRequest{false};

        void incrementScrollX();
        void incrementScrollY();
        void transferAddressX();
        void transferAddressY();

        void loadBackgroundShifters();
        void updateShifters();

        void createWindow(const char *title, int width, int height);
        void destroyWindow() noexcept;
    };
} // namespace NES

#endif // NES_PPU_H