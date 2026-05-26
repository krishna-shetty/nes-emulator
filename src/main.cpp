#include "nes_ppu.h"
#include <iostream>

int main()
{
    NES::PPU ppu("NES Emulator", 768, 480);

    while (ppu.isRunning())
    {
        ppu.handleEvents();
        ppu.clear();
        ppu.present();
    }

    return 0;
}