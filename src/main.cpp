#include "nes_cpu.h"
#include "nes_ppu.h"
#include "nes_debugger.h"

int main()
{
    NES::Bus bus;
    NES::CPU cpu(bus);
    NES::PPU ppu("NES Emulator", 768, 480);
    NES::Debugger debugger(cpu, ppu, bus);

    // Load a tiny test program
    bus.loadProgram(0x0000, {0xA9, 0x42, 0xE8, 0x4C, 0x02, 0x00}); // LDA #$42, INX, JMP $0002
    cpu.reset();
    cpu.setPC(0x0000);

    while (ppu.isRunning())
    {
        ppu.handleEvents([&](SDL_Event &event)
                         { ImGui_ImplSDL3_ProcessEvent(&event); });
        ppu.clear();
        debugger.render();
        ppu.present();
    }

    return 0;
}