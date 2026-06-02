#include "nes_debugger.h"
#include <cstdio>
#include <stdexcept>

using namespace NES;


enum class AddrMode : uint8_t
{
    IMP, ACC, IMM, ZP, ZPX, ZPY,
    ABS, ABX, ABY, IND, IZX, IZY, REL
};

struct InstructionInfo
{
    const char* mnemonic;
    AddrMode    mode;
};

// Indexed by opcode byte
static const InstructionInfo TABLE[256] =
{
    /* 0x00 */ {"BRK", AddrMode::IMP}, {"ORA", AddrMode::IZX}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x04 */ {"???", AddrMode::IMP}, {"ORA", AddrMode::ZP},  {"ASL", AddrMode::ZP},  {"???", AddrMode::IMP},
    /* 0x08 */ {"PHP", AddrMode::IMP}, {"ORA", AddrMode::IMM}, {"ASL", AddrMode::ACC}, {"???", AddrMode::IMP},
    /* 0x0C */ {"???", AddrMode::IMP}, {"ORA", AddrMode::ABS}, {"ASL", AddrMode::ABS}, {"???", AddrMode::IMP},

    /* 0x10 */ {"BPL", AddrMode::REL}, {"ORA", AddrMode::IZY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x14 */ {"???", AddrMode::IMP}, {"ORA", AddrMode::ZPX}, {"ASL", AddrMode::ZPX}, {"???", AddrMode::IMP},
    /* 0x18 */ {"CLC", AddrMode::IMP}, {"ORA", AddrMode::ABY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x1C */ {"???", AddrMode::IMP}, {"ORA", AddrMode::ABX}, {"ASL", AddrMode::ABX}, {"???", AddrMode::IMP},

    /* 0x20 */ {"JSR", AddrMode::ABS}, {"AND", AddrMode::IZX}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x24 */ {"BIT", AddrMode::ZP},  {"AND", AddrMode::ZP},  {"ROL", AddrMode::ZP},  {"???", AddrMode::IMP},
    /* 0x28 */ {"PLP", AddrMode::IMP}, {"AND", AddrMode::IMM}, {"ROL", AddrMode::ACC}, {"???", AddrMode::IMP},
    /* 0x2C */ {"BIT", AddrMode::ABS}, {"AND", AddrMode::ABS}, {"ROL", AddrMode::ABS}, {"???", AddrMode::IMP},

    /* 0x30 */ {"BMI", AddrMode::REL}, {"AND", AddrMode::IZY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x34 */ {"???", AddrMode::IMP}, {"AND", AddrMode::ZPX}, {"ROL", AddrMode::ZPX}, {"???", AddrMode::IMP},
    /* 0x38 */ {"SEC", AddrMode::IMP}, {"AND", AddrMode::ABY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x3C */ {"???", AddrMode::IMP}, {"AND", AddrMode::ABX}, {"ROL", AddrMode::ABX}, {"???", AddrMode::IMP},

    /* 0x40 */ {"RTI", AddrMode::IMP}, {"EOR", AddrMode::IZX}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x44 */ {"???", AddrMode::IMP}, {"EOR", AddrMode::ZP},  {"LSR", AddrMode::ZP},  {"???", AddrMode::IMP},
    /* 0x48 */ {"PHA", AddrMode::IMP}, {"EOR", AddrMode::IMM}, {"LSR", AddrMode::ACC}, {"???", AddrMode::IMP},
    /* 0x4C */ {"JMP", AddrMode::ABS}, {"EOR", AddrMode::ABS}, {"LSR", AddrMode::ABS}, {"???", AddrMode::IMP},

    /* 0x50 */ {"BVC", AddrMode::REL}, {"EOR", AddrMode::IZY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x54 */ {"???", AddrMode::IMP}, {"EOR", AddrMode::ZPX}, {"LSR", AddrMode::ZPX}, {"???", AddrMode::IMP},
    /* 0x58 */ {"CLI", AddrMode::IMP}, {"EOR", AddrMode::ABY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x5C */ {"???", AddrMode::IMP}, {"EOR", AddrMode::ABX}, {"LSR", AddrMode::ABX}, {"???", AddrMode::IMP},

    /* 0x60 */ {"RTS", AddrMode::IMP}, {"ADC", AddrMode::IZX}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x64 */ {"???", AddrMode::IMP}, {"ADC", AddrMode::ZP},  {"ROR", AddrMode::ZP},  {"???", AddrMode::IMP},
    /* 0x68 */ {"PLA", AddrMode::IMP}, {"ADC", AddrMode::IMM}, {"ROR", AddrMode::ACC}, {"???", AddrMode::IMP},
    /* 0x6C */ {"JMP", AddrMode::IND}, {"ADC", AddrMode::ABS}, {"ROR", AddrMode::ABS}, {"???", AddrMode::IMP},

    /* 0x70 */ {"BVS", AddrMode::REL}, {"ADC", AddrMode::IZY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x74 */ {"???", AddrMode::IMP}, {"ADC", AddrMode::ZPX}, {"ROR", AddrMode::ZPX}, {"???", AddrMode::IMP},
    /* 0x78 */ {"SEI", AddrMode::IMP}, {"ADC", AddrMode::ABY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x7C */ {"???", AddrMode::IMP}, {"ADC", AddrMode::ABX}, {"ROR", AddrMode::ABX}, {"???", AddrMode::IMP},

    /* 0x80 */ {"???", AddrMode::IMP}, {"STA", AddrMode::IZX}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x84 */ {"STY", AddrMode::ZP},  {"STA", AddrMode::ZP},  {"STX", AddrMode::ZP},  {"???", AddrMode::IMP},
    /* 0x88 */ {"DEY", AddrMode::IMP}, {"???", AddrMode::IMP}, {"TXA", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x8C */ {"STY", AddrMode::ABS}, {"STA", AddrMode::ABS}, {"STX", AddrMode::ABS}, {"???", AddrMode::IMP},

    /* 0x90 */ {"BCC", AddrMode::REL}, {"STA", AddrMode::IZY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x94 */ {"STY", AddrMode::ZPX}, {"STA", AddrMode::ZPX}, {"STX", AddrMode::ZPY}, {"???", AddrMode::IMP},
    /* 0x98 */ {"TYA", AddrMode::IMP}, {"STA", AddrMode::ABY}, {"TXS", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0x9C */ {"???", AddrMode::IMP}, {"STA", AddrMode::ABX}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},

    /* 0xA0 */ {"LDY", AddrMode::IMM}, {"LDA", AddrMode::IZX}, {"LDX", AddrMode::IMM}, {"???", AddrMode::IMP},
    /* 0xA4 */ {"LDY", AddrMode::ZP},  {"LDA", AddrMode::ZP},  {"LDX", AddrMode::ZP},  {"???", AddrMode::IMP},
    /* 0xA8 */ {"TAY", AddrMode::IMP}, {"LDA", AddrMode::IMM}, {"TAX", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xAC */ {"LDY", AddrMode::ABS}, {"LDA", AddrMode::ABS}, {"LDX", AddrMode::ABS}, {"???", AddrMode::IMP},

    /* 0xB0 */ {"BCS", AddrMode::REL}, {"LDA", AddrMode::IZY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xB4 */ {"LDY", AddrMode::ZPX}, {"LDA", AddrMode::ZPX}, {"LDX", AddrMode::ZPY}, {"???", AddrMode::IMP},
    /* 0xB8 */ {"CLV", AddrMode::IMP}, {"LDA", AddrMode::ABY}, {"TSX", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xBC */ {"LDY", AddrMode::ABX}, {"LDA", AddrMode::ABX}, {"LDX", AddrMode::ABY}, {"???", AddrMode::IMP},

    /* 0xC0 */ {"CPY", AddrMode::IMM}, {"CMP", AddrMode::IZX}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xC4 */ {"CPY", AddrMode::ZP},  {"CMP", AddrMode::ZP},  {"DEC", AddrMode::ZP},  {"???", AddrMode::IMP},
    /* 0xC8 */ {"INY", AddrMode::IMP}, {"CMP", AddrMode::IMM}, {"DEX", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xCC */ {"CPY", AddrMode::ABS}, {"CMP", AddrMode::ABS}, {"DEC", AddrMode::ABS}, {"???", AddrMode::IMP},

    /* 0xD0 */ {"BNE", AddrMode::REL}, {"CMP", AddrMode::IZY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xD4 */ {"???", AddrMode::IMP}, {"CMP", AddrMode::ZPX}, {"DEC", AddrMode::ZPX}, {"???", AddrMode::IMP},
    /* 0xD8 */ {"CLD", AddrMode::IMP}, {"CMP", AddrMode::ABY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xDC */ {"???", AddrMode::IMP}, {"CMP", AddrMode::ABX}, {"DEC", AddrMode::ABX}, {"???", AddrMode::IMP},

    /* 0xE0 */ {"CPX", AddrMode::IMM}, {"SBC", AddrMode::IZX}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xE4 */ {"CPX", AddrMode::ZP},  {"SBC", AddrMode::ZP},  {"INC", AddrMode::ZP},  {"???", AddrMode::IMP},
    /* 0xE8 */ {"INX", AddrMode::IMP}, {"SBC", AddrMode::IMM}, {"NOP", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xEC */ {"CPX", AddrMode::ABS}, {"SBC", AddrMode::ABS}, {"INC", AddrMode::ABS}, {"???", AddrMode::IMP},

    /* 0xF0 */ {"BEQ", AddrMode::REL}, {"SBC", AddrMode::IZY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xF4 */ {"???", AddrMode::IMP}, {"SBC", AddrMode::ZPX}, {"INC", AddrMode::ZPX}, {"???", AddrMode::IMP},
    /* 0xF8 */ {"SED", AddrMode::IMP}, {"SBC", AddrMode::ABY}, {"???", AddrMode::IMP}, {"???", AddrMode::IMP},
    /* 0xFC */ {"???", AddrMode::IMP}, {"SBC", AddrMode::ABX}, {"INC", AddrMode::ABX}, {"???", AddrMode::IMP},
};

// Returns instruction size in bytes (1, 2, or 3)
static uint8_t instructionSize(AddrMode mode)
{
    switch (mode)
    {
        case AddrMode::IMP:
        case AddrMode::ACC:
            return 1;
        case AddrMode::IMM:
        case AddrMode::ZP:
        case AddrMode::ZPX:
        case AddrMode::ZPY:
        case AddrMode::IZX:
        case AddrMode::IZY:
        case AddrMode::REL:
            return 2;
        case AddrMode::ABS:
        case AddrMode::ABX:
        case AddrMode::ABY:
        case AddrMode::IND:
            return 3;
    }
    return 1;
}

// ----------------------------------------------------------------------------
// Debugger
// ----------------------------------------------------------------------------

Debugger::Debugger(Emulator &emulator)
    : _cpu(emulator.getCPU())
    , _ppu(emulator.getPPU())
    , _bus(emulator.getBus())
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(_ppu.getWindow(), _ppu.getRenderer());
    ImGui_ImplSDLRenderer3_Init(_ppu.getRenderer());
}

Debugger::~Debugger() noexcept
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

Debugger::DisassembledInstruction Debugger::disassemble(uint16_t address) const
{
    uint8_t opcode        = _bus.peek(address);
    uint8_t lo            = _bus.peek(address + 1);
    uint8_t hi            = _bus.peek(address + 2);
    uint16_t abs          = static_cast<uint16_t>((hi << 8) | lo);

    const InstructionInfo& info = TABLE[opcode];
    char buf[64];

    switch (info.mode)
    {
        case AddrMode::IMP: snprintf(buf, sizeof(buf), "%s",            info.mnemonic);                   break;
        case AddrMode::ACC: snprintf(buf, sizeof(buf), "%s A",          info.mnemonic);                   break;
        case AddrMode::IMM: snprintf(buf, sizeof(buf), "%s #$%02X",     info.mnemonic, lo);               break;
        case AddrMode::ZP:  snprintf(buf, sizeof(buf), "%s $%02X",      info.mnemonic, lo);               break;
        case AddrMode::ZPX: snprintf(buf, sizeof(buf), "%s $%02X,X",    info.mnemonic, lo);               break;
        case AddrMode::ZPY: snprintf(buf, sizeof(buf), "%s $%02X,Y",    info.mnemonic, lo);               break;
        case AddrMode::ABS: snprintf(buf, sizeof(buf), "%s $%04X",      info.mnemonic, abs);              break;
        case AddrMode::ABX: snprintf(buf, sizeof(buf), "%s $%04X,X",    info.mnemonic, abs);              break;
        case AddrMode::ABY: snprintf(buf, sizeof(buf), "%s $%04X,Y",    info.mnemonic, abs);              break;
        case AddrMode::IND: snprintf(buf, sizeof(buf), "%s ($%04X)",    info.mnemonic, abs);              break;
        case AddrMode::IZX: snprintf(buf, sizeof(buf), "%s ($%02X,X)",  info.mnemonic, lo);               break;
        case AddrMode::IZY: snprintf(buf, sizeof(buf), "%s ($%02X),Y",  info.mnemonic, lo);               break;
        case AddrMode::REL:
        {
            int8_t  offset = static_cast<int8_t>(lo);
            uint16_t target = (address + 2) + offset;
            snprintf(buf, sizeof(buf), "%s $%04X", info.mnemonic, target);
            break;
        }
    }

    return {address, buf};
}

void Debugger::render()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    CPU::State state = _cpu.getState();

    // -------------------------------------------------------------------------
    // CPU Registers
    // -------------------------------------------------------------------------
    ImGui::SetNextWindowPos(ImVec2(320, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("CPU");


    ImGui::Text("A:  $%02X  (%3d)", state.A, state.A);
    ImGui::Text("X:  $%02X  (%3d)", state.X, state.X);
    ImGui::Text("Y:  $%02X  (%3d)", state.Y, state.Y);
    ImGui::Text("PC: $%04X",        state.PC);
    ImGui::Text("SP: $%02X",        state.SP);
    ImGui::Text("Cycles: %llu",     state.cycles);

    ImGui::Separator();

    ImGui::Text("Flags:");
    ImGui::SameLine();

    for (int i = 7; i >= 0; i--)
    {
        bool set = (state.status >> i) & 1;
        if (set)
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%c", "NVUBDIZC"[7 - i]);
        else
            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "%c", "nvubdizc"[7 - i]);
        if (i != 0) ImGui::SameLine();
    }

    ImGui::Separator();

    if (ImGui::Button("Step"))
        _cpu.clock();

    ImGui::End();

    // -------------------------------------------------------------------------
    // Disassembly
    // -------------------------------------------------------------------------
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Disassembly");

    uint16_t pc = state.PC;
    for (int i = 0; i < 16; i++)
    {
        auto instr = disassemble(pc);
        uint8_t opcode = _bus.peek(pc);
        uint8_t size   = instructionSize(TABLE[opcode].mode);

        if (pc == state.PC)
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "-> $%04X: %s", instr.address, instr.text.c_str());
        else
            ImGui::Text("   $%04X: %s", instr.address, instr.text.c_str());

        pc += size;
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), _ppu.getRenderer());
}