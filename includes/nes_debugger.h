#ifndef NES_DEBUGGER_H
#define NES_DEBUGGER_H

#include "nes_cpu.h"
#include "nes_ppu.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlrenderer3.h"

namespace NES
{
    class Debugger
    {
    public:
        Debugger(CPU &cpu, PPU &ppu, Bus &bus);
        ~Debugger() noexcept;

        Debugger(const Debugger &) = delete;
        Debugger &operator=(const Debugger &) = delete;
        Debugger(Debugger &&) = delete;
        Debugger &operator=(Debugger &&) = delete;

        void render();

    private:
        CPU& _cpu;
        PPU& _ppu;
        Bus& _bus;

        struct DisassembledInstruction
        {
            uint16_t address;
            std::string text;
        };

        DisassembledInstruction disassemble(uint16_t address) const;
    };
} // namespace NES

#endif // NES_DEBUGGER_H