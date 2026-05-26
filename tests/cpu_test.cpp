#include <gtest/gtest.h>
#include "nes_cpu.h"

using namespace NES;

// Helper to create a CPU with a program loaded at $0000
static CPU makeTestCPU(std::vector<uint8_t> program)
{
    CPU cpu;
    cpu.loadProgram(0x0000, program);
    cpu.reset();
    return cpu;
}

// ============================================================
// LDA - Load Accumulator
// ============================================================

TEST(LDA, Immediate_LoadsValue)
{
    CPU cpu = makeTestCPU({0xA9, 0x42}); // LDA #$42
    cpu.step();
    EXPECT_EQ(cpu.getState().A, 0x42);
}

TEST(LDA, Immediate_SetsZeroFlag)
{
    CPU cpu = makeTestCPU({0xA9, 0x00}); // LDA #$00
    cpu.step();
    EXPECT_EQ(cpu.getState().A, 0x00);
    EXPECT_TRUE(cpu.getState().status & 0x02); // Z flag
}

TEST(LDA, Immediate_SetsNegativeFlag)
{
    CPU cpu = makeTestCPU({0xA9, 0x80}); // LDA #$80
    cpu.step();
    EXPECT_TRUE(cpu.getState().status & 0x80); // N flag
}

TEST(LDA, Immediate_CycleCount)
{
    CPU cpu = makeTestCPU({0xA9, 0x42}); // LDA #$42
    cpu.step();
    EXPECT_EQ(cpu.getState().cycles, 2);
}

// ============================================================
// ADC - Add with Carry
// ============================================================

TEST(ADC, Immediate_AddsToAccumulator)
{
    CPU cpu = makeTestCPU({0xA9, 0x10, 0x69, 0x20}); // LDA #$10, ADC #$20
    cpu.step(); // LDA
    cpu.step(); // ADC
    EXPECT_EQ(cpu.getState().A, 0x30);
}

TEST(ADC, Immediate_SetsCarryOnOverflow)
{
    CPU cpu = makeTestCPU({0xA9, 0xFF, 0x69, 0x01}); // LDA #$FF, ADC #$01
    cpu.step();
    cpu.step();
    EXPECT_EQ(cpu.getState().A, 0x00);
    EXPECT_TRUE(cpu.getState().status & 0x01); // C flag
}

TEST(ADC, Immediate_CycleCount)
{
    CPU cpu = makeTestCPU({0xA9, 0x10, 0x69, 0x20});
    cpu.step(); // LDA - 2 cycles
    cpu.step(); // ADC - 2 cycles
    EXPECT_EQ(cpu.getState().cycles, 4);
}

// ============================================================
// INX / INY
// ============================================================

TEST(INX, IncrementsX)
{
    CPU cpu = makeTestCPU({0xA2, 0x05, 0xE8}); // LDX #$05, INX
    cpu.step();
    cpu.step();
    EXPECT_EQ(cpu.getState().X, 0x06);
}

TEST(INX, WrapsAround)
{
    CPU cpu = makeTestCPU({0xA2, 0xFF, 0xE8}); // LDX #$FF, INX
    cpu.step();
    cpu.step();
    EXPECT_EQ(cpu.getState().X, 0x00);
    EXPECT_TRUE(cpu.getState().status & 0x02); // Z flag
}

// ============================================================
// JMP
// ============================================================

TEST(JMP, Absolute_SetsPC)
{
    CPU cpu = makeTestCPU({0x4C, 0x05, 0x00}); // JMP $0005
    cpu.step();
    EXPECT_EQ(cpu.getState().PC, 0x0005);
}