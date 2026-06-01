#include <iostream>
#include <memory>
#include <filesystem>
#include <fstream>

#include "nes.h"
#include "nes_debugger.h"
#include "imgui/imgui_impl_sdl3.h"

int main()
{
    try
    {
        NES::Emulator nes("NES Emulator", 768, 720);
        NES::Debugger debugger(nes);

        auto cartridge = std::make_shared<NES::Cartridge>("roms/dk.nes");
        nes.insertCartridge(cartridge);

        nes.reset();

        while (nes.isRunning())
        {
            nes.handleEvents([&](SDL_Event& event)
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
            });

            nes.tick([&]() {
                debugger.render();
            });
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << "\n";
        std::cin.get();
        return 1;
    }

    return 0;
}