# NES Emulator

... or the makings of one, at least.

## What's Here

- **Cycle-accurate 6502 CPU** with all official opcodes and addressing modes
- **Per-opcode GoogleTest suite:** Assertions on register state, flags, and cycle counts
- **ImGui debugger:** Register inspection, status flags, disassembly view, and single-step execution
- **SDL3 windowing** via a stub PPU

## What's Amiss

- PPU (graphics): the window initializes but no rendering beyond the debugger overlay
- APU (audio)
- Cartridge/mapper support

## Dependencies

- [SDL3](https://github.com/libsdl-org/SDL)
- [GoogleTest](https://github.com/google/googletest): for running CPU tests only
- ImGui: included in source

Install on macOS:
```bash
brew install sdl3 googletest
```

## Build

**Emulator:**
```bash
mkdir -p build && clang++ -std=c++17 -Wall -Wextra -g \
  $(find src -name '*.cpp') \
  -Iincludes -Iincludes/imgui \
  $(pkg-config --cflags sdl3) \
  $(pkg-config --libs sdl3) \
  -o build/nes
./build/nes
```

**Tests:**
```bash
mkdir -p build && clang++ -std=c++17 -g \
  src/nes_ram.cpp src/nes_cpu.cpp \
  tests/cpu_test.cpp \
  -Iincludes \
  -I/opt/homebrew/include \
  -L/opt/homebrew/lib \
  -lgtest -lgtest_main \
  -o build/nes_test
./build/nes_test
```

## Notes

No CMake! Build instructions are explicit by design. If you're on a different platform, the clang commands translate directly to your toolchain of choice.