#ifndef NES_CPU_H
#define NES_CPU_H

#include "nes.h"
#include "nes_ram.h"
#include <cstdint>

namespace NES
{
    class CPU
    {
    public:
        
        CPU(Region region = Region::NTSC);

    private:
        uint8_t _A;
        uint8_t _X;
        uint8_t _Y;
        uint16_t _pc;
        uint8_t _sp;
        uint8_t _status;
        uint32_t CLOCK_FREQUENCY{1790000};
        RAM _ram;

        uint64_t _cycles {0};

        enum class Flags : uint8_t {
            C = 0,  // Carry
            Z = 1,  // Zero
            I = 2,  // Interrupt Disable
            D = 3,  // Decimal (unused on NES)
            B = 4,  // Break
            U = 5,  // Unused
            V = 6,  // Overflow
            N = 7,  // Negative
        };

        void setFlag(Flags flag, bool value);
        void setAccumulator(uint8_t value);
        void setX(uint8_t value);
        void setY(uint8_t value);
        uint8_t getFlag(Flags flag);
        void branch(int8_t offset, bool condition);
        void push(uint8_t value);
        uint8_t pull();

        // Addressing modes
        uint8_t     addrAccumulator();
        uint16_t    addrAbsolute();
        uint16_t    addrAbsoluteX();
        uint16_t    addrAbsoluteY();
        uint16_t     addrImmediate();
        void        addrImplied();
        uint16_t    addrIndirect();
        uint16_t    addrIndirectX();
        uint16_t    addrIndirectY();
        uint16_t    addrZeroPage();
        uint16_t    addrZeroPageX();
        uint16_t    addrZeroPageY();

        // Official instructions
        void ADC(uint8_t memory);
        void AND(uint8_t memory);
        uint8_t ASL(uint8_t value);
        void BCC(int8_t memory);
        void BCS(int8_t memory);
        void BEQ(int8_t memory);
        void BIT(uint8_t memory);
        void BMI(int8_t memory);
        void BNE(int8_t memory);
        void BPL(int8_t memory);
        void BRK();
        void BVC(int8_t memory);
        void BVS(int8_t memory);
        void CLC();
        void CLD();
        void CLI();
        void CLV();
        void CMP(uint8_t memory);
        void CPX(uint8_t memory);
        void CPY(uint8_t memory);
        uint8_t DEC(uint8_t memory);
        void DEX();
        void DEY();
        void EOR(uint8_t memory);
        uint8_t INC(uint8_t memory);
        void INX();
        void INY();
        void JMP(uint16_t memory);
        void JSR(uint8_t memory);
        void LDA(uint8_t memory);
        void LDX(uint8_t memory);
        void LDY(uint8_t memory);
        uint8_t LSR(uint8_t value);
        void NOP();
        void ORA(uint8_t memory);
        void PHA();
        void PHP();
        void PLA();
        void PLP();
        uint8_t ROL(uint8_t value);
        uint8_t ROR(uint8_t value);
        void RTI();
        void RTS();
        void SBC(uint8_t memory);
        void SEC();
        void SED();
        void SEI();
        void STA(uint16_t memory);
        void STX(uint16_t memory);
        void STY(uint16_t memory);
        void TAX();
        void TAY();
        void TSX();
        void TXA();
        void TXS();
        void TYA();

        inline uint32_t getClockFrequency(Region region)
        {
            switch(region)
            {
                case Region::NTSC:  return 1'789'773;
                case Region::PAL:   return 1'662'607;
            }
        }

        inline void incrementCycles(uint8_t increment = 1)
        {
            _cycles += increment;
        }
    };
} // namespace NES
#endif // NES_CPU_H