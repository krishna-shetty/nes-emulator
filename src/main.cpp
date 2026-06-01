#include <iostream>
#include <memory>

#include "nes.h"
#include "nes_debugger.h"
#include "imgui/imgui_impl_sdl3.h"

int main()
{
    try
    {
        NES::Emulator nes("NES Emulator", 768, 480);
        NES::Debugger debugger(nes);

        auto cartridge = std::make_shared<NES::Cartridge>("roms/nestest.nes");
        nes.insertCartridge(cartridge);

        nes.reset();

        while (nes.isRunning())
        {
            nes.handleEvents(
                [&](SDL_Event& event)
                {
                    ImGui_ImplSDL3_ProcessEvent(&event);
                });

            nes.tick(
                [&]()
                {
                    debugger.render();
                });
        }
    }
    catch (const std::exception &e)
    {
        SDL_Log("Fatal: %s", e.what());
        return 1;
    }
    catch (...)
    {
        SDL_Log("Fatal: unknown exception");
        return 1;
    }

    return 0;
}