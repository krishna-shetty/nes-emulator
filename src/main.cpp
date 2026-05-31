#include "nes.h"
#include "nes_debugger.h"
#include "imgui/imgui_impl_sdl3.h"

int main()
{
    NES::Emulator nes("NES Emulator", 768, 480);
    NES::Debugger debugger(nes);

    nes.getBus().loadProgram(0x0000, {0xA9, 0x42, 0xE8, 0x4C, 0x02, 0x00});
    nes.reset();
    nes.getCPU().setPC(0x0000);

    while (nes.isRunning())
    {
        nes.getPPU().handleEvents([&](SDL_Event &event)
                                  { ImGui_ImplSDL3_ProcessEvent(&event); });

        nes.tick([&]()
                 { debugger.render(); });
    }

    return 0;
}