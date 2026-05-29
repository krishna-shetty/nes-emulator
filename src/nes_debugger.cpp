#include "nes_debugger.h"
#include <cstdio>

using namespace NES;

Debugger::Debugger(CPU& cpu, PPU& ppu, Bus& bus)
    : _cpu(cpu), _ppu(ppu), _bus(bus)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForSDLRenderer(_ppu.getWindow(), _ppu.getRenderer()))
    {
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialize ImGui SDL3 backend");
    }

    if (!ImGui_ImplSDLRenderer3_Init(_ppu.getRenderer()))
    {
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialize ImGui SDL3 renderer backend");
    }
}

Debugger::~Debugger() noexcept
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

Debugger::DisassembledInstruction Debugger::disassemble(uint16_t address) const
{
    uint8_t opcode = _bus.read(address);
    char buf[32];

    switch (opcode)
    {
        case 0xA9: snprintf(buf, sizeof(buf), "LDA #$%02X",     _bus.peek(address + 1)); return {address, buf};
        case 0xA5: snprintf(buf, sizeof(buf), "LDA $%02X",      _bus.peek(address + 1)); return {address, buf};
        case 0xAD: snprintf(buf, sizeof(buf), "LDA $%02X%02X",  _bus.peek(address + 2), _bus.peek(address + 1)); return {address, buf};
        case 0xA2: snprintf(buf, sizeof(buf), "LDX #$%02X",     _bus.peek(address + 1)); return {address, buf};
        case 0xA0: snprintf(buf, sizeof(buf), "LDY #$%02X",     _bus.peek(address + 1)); return {address, buf};
        case 0x69: snprintf(buf, sizeof(buf), "ADC #$%02X",     _bus.peek(address + 1)); return {address, buf};
        case 0x65: snprintf(buf, sizeof(buf), "ADC $%02X",      _bus.peek(address + 1)); return {address, buf};
        case 0x85: snprintf(buf, sizeof(buf), "STA $%02X",      _bus.peek(address + 1)); return {address, buf};
        case 0x8D: snprintf(buf, sizeof(buf), "STA $%02X%02X",  _bus.peek(address + 2), _bus.peek(address + 1)); return {address, buf};
        case 0x4C: snprintf(buf, sizeof(buf), "JMP $%02X%02X",  _bus.peek(address + 2), _bus.peek(address + 1)); return {address, buf};
        case 0x6C: snprintf(buf, sizeof(buf), "JMP ($%02X%02X)",_bus.peek(address + 2), _bus.peek(address + 1)); return {address, buf};
        case 0x20: snprintf(buf, sizeof(buf), "JSR $%02X%02X",  _bus.peek(address + 2), _bus.peek(address + 1)); return {address, buf};
        case 0x60: snprintf(buf, sizeof(buf), "RTS",            _bus.peek(address + 1)); return {address, buf};
        case 0xE8: return {address, "INX"};
        case 0xC8: return {address, "INY"};
        case 0xCA: return {address, "DEX"};
        case 0x88: return {address, "DEY"};
        case 0xEA: return {address, "NOP"};
        case 0x00: return {address, "BRK"};
        case 0x18: return {address, "CLC"};
        case 0x38: return {address, "SEC"};
        case 0x58: return {address, "CLI"};
        case 0x78: return {address, "SEI"};
        case 0xD8: return {address, "CLD"};
        case 0xF8: return {address, "SED"};
        case 0xB8: return {address, "CLV"};
        default:   snprintf(buf, sizeof(buf), "??? ($%02X)", opcode); return {address, buf};
    }
}

void Debugger::render()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    CPU::State state = _cpu.getState();

    ImGui::Begin("CPU Debugger");

    // Registers
    ImGui::Text("A:  $%02X  (%d)", state.A, state.A);
    ImGui::Text("X:  $%02X  (%d)", state.X, state.X);
    ImGui::Text("Y:  $%02X  (%d)", state.Y, state.Y);
    ImGui::Text("PC: $%04X", state.PC);
    ImGui::Text("SP: $%02X", state.SP);
    ImGui::Text("Cycles: %llu", state.cycles);

    ImGui::Separator();

    // Status flags
    ImGui::Text("Flags: %c%c%c%c%c%c%c%c",
        (state.status & 0x80) ? 'N' : 'n',
        (state.status & 0x40) ? 'V' : 'v',
        (state.status & 0x20) ? 'U' : 'u',
        (state.status & 0x10) ? 'B' : 'b',
        (state.status & 0x08) ? 'D' : 'd',
        (state.status & 0x04) ? 'I' : 'i',
        (state.status & 0x02) ? 'Z' : 'z',
        (state.status & 0x01) ? 'C' : 'c');

    ImGui::Separator();

    // Step button
    if (ImGui::Button("Step"))
    {
        _cpu.step();
    }

    ImGui::Separator();
ImGui::Text("Disassembly:");

uint16_t pc = state.PC;
for (int i = 0; i < 5; i++)
{
    auto instr = disassemble(pc);
    if (pc == state.PC)
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "-> $%04X: %s", instr.address, instr.text.c_str());
    else
        ImGui::Text("   $%04X: %s", instr.address, instr.text.c_str());

    // Advance pc by instruction size
    uint8_t opcode = _bus.peek(pc);
    switch (opcode)
    {
        case 0xA9: case 0xA2: case 0xA0:
        case 0x69: case 0x65: case 0xE9:
        case 0x00:
            pc += 2; break;
        case 0xAD: case 0x8D: case 0x4C:
        case 0x6C: case 0x20:
            pc += 3; break;
        default:
            pc += 1; break;
    }
}

    ImGui::End();

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), _ppu.getRenderer());
}