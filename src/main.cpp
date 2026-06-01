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

        auto cartridge = std::make_shared<NES::Cartridge>("roms/donkey_kong.nes");
        nes.insertCartridge(cartridge);

        nes.reset();

        auto state = nes.getCPU().getState();

        while (nes.isRunning())
        {
            nes.getPPU().handleEvents(
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
    catch (const std::exception& e)
    {
        std::cerr << "\n========================\n";
        std::cerr << "EXCEPTION CAUGHT\n";
        std::cerr << e.what() << '\n';
        std::cerr << "========================\n";

        std::cin.get(); // keep terminal open
        return 1;
    }
    catch (...)
    {
        std::cerr << "\nUNKNOWN EXCEPTION\n";
        std::cin.get();
        return 1;
    }

    return 0;
}