# NES Emulator

... emulates a NES like it's a NES, minus the audio and most of the library (hee hee).

## Features

- **Cycle-accurate 6502 CPU** with all official opcodes and addressing modes
- **PPU implementation** with background rendering, sprite rendering, scrolling, DMA, and NMI support
- **Mapper 000 (NROM)** cartridge support
- **Controller input** ($4016/$4017)
- **ImGui debugger** with register inspection, status flags, memory view, and single-step execution
- **Integrated disassembler**
- **GoogleTest CPU test suite** with per-opcode validation

## Roadmap

- APU (audio)
- Additional mapper support
- Accuracy improvements and hardware edge cases

## Tested Games

- nestest
- Donkey Kong
- Super Mario Bros.

## Dependencies

- [SDL3](https://github.com/libsdl-org/SDL)
- [GoogleTest](https://github.com/google/googletest) (tests only)
- ImGui (included in source)

Install on macOS:

```bash
brew install sdl3 googletest
```

## Build

### Emulator

```bash
mkdir -p build && clang++ -std=c++17 -Wall -Wextra -g \
  $(find src -name '*.cpp') \
  -Iincludes -Iincludes/imgui \
  $(pkg-config --cflags sdl3) \
  $(pkg-config --libs sdl3) \
  -o build/nes

./build/nes
```

### Tests

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

No CMake. Build instructions are explicit by design.

The emulator currently targets NROM cartridges and is being developed with an emphasis on correctness and understanding of NES hardware internals.