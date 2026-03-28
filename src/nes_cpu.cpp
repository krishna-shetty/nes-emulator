#include "nes_cpu.h"

using namespace NES;

CPU::CPU(Region region = Region::NTSC)
{
    CLOCK_FREQUENCY = getClockFrequency(region);
    _ram = RAM();
}

void CPU::setFlag(Flags flag, bool value)
{
    uint8_t n = static_cast<uint8_t>(flag);
    value ? setBit(_status, n) : clearBit(_status, n);
}

uint8_t CPU::getFlag(Flags flag)
{
    uint8_t n = static_cast<uint8_t>(flag);
    return getBit(_status, n);
}

void CPU::setAccumulator(uint8_t value)
{
    setFlag(Flags::Z, value == 0);
    setFlag(Flags::N, getBit(value, 7)); // result bit 7
    _A = value;
}

void CPU::setX(uint8_t value)
{
    setFlag(Flags::Z, value == 0);
    setFlag(Flags::N, getBit(value, 7)); // result bit 7
    _X = value;
}

void CPU::push(uint8_t value)
{
    _ram.write(0x0100 | _sp, value);
    _sp--;
}

uint8_t CPU::pull()
{
    _sp++;
    return _ram.read(0x0100 | _sp);
}

// Add with Carry
void CPU::ADC(uint8_t memory)
{
    uint16_t sum = _A + memory + getFlag(Flags::C);
    uint8_t result = static_cast<uint8_t>(sum);

    /* If the result overflowed past $FF (wrapping around),
    unsigned overflow occurred. */
    setFlag(Flags::C, sum > 0xFF);

    /* If the result's sign is different from both A's and memory's
    signed overflow (or underflow) occurred. */
    setFlag(Flags::V, (sum ^ _A) & (sum ^ memory) & 0x80);

    setAccumulator(result);
}

// Bitwise AND
void CPU::AND(uint8_t memory)
{
    uint8_t result = _A & memory;

    setAccumulator(result);
}

// Arithmetic Shift Left
uint8_t CPU::ASL(uint8_t value)
{
    uint8_t carry = getBit(value, 7); // value bit 7
    value <<= 1;

    setFlag(Flags::C, carry == 1);
    setFlag(Flags::Z, value == 0);
    setFlag(Flags::N, getBit(value, 7)); // result bit 7

    return value;
}

void CPU::branch(int8_t offset, bool condition)
{
    if (condition)
    {
        incrementCycles();
        _pc += offset;
    }
}

// Branch if Carry Clear
void CPU::BCC(int8_t memory)
{
    branch(memory, getFlag(Flags::C) == 0);    
}

// Branch if Carry Set
void CPU::BCS(int8_t memory)
{
   branch(memory, getFlag(Flags::C) == 1);       
}

// Branch if Equal
void CPU::BEQ(int8_t memory)
{
   branch(memory, getFlag(Flags::Z) == 1);      
}

// Bit Test
void CPU::BIT(uint8_t memory)
{
    uint8_t result = _A & memory;

    setFlag(Flags::Z, result == 0);
    setFlag(Flags::V, getBit(memory, 6)); // memory bit 6
    setFlag(Flags::N, getBit(memory, 7)); // memory bit 7
}

// Branch if Minus
void CPU::BMI(int8_t memory)
{
    branch(memory, getFlag(Flags::N) == 1);
}

// Branch if Not Equal
void CPU::BNE(int8_t memory)
{
    branch(memory, getFlag(Flags::Z) == 0);
}

// Branch if Plus
void CPU::BPL(int8_t memory)
{
    branch(memory, getFlag(Flags::N) == 0);
}

void CPU::BRK()
{
    uint16_t returnAddr = _pc + 1;

    push((returnAddr >> 8) & 0xFF);
    push(returnAddr & 0xFF);

    uint8_t pushedStatus = _status;
    setBit(pushedStatus, static_cast<uint8_t>(Flags::B));
    setBit(pushedStatus, static_cast<uint8_t>(Flags::U));
    push(pushedStatus);

    setFlag(Flags::I, true);

    uint8_t lo = _ram.read(0xFFFE);
    uint8_t hi = _ram.read(0xFFFF);
    _pc = (static_cast<uint16_t>(hi) << 8) | lo;
}

// Branch if Overflow Clear
void CPU::BVC(int8_t memory)
{
    branch(memory, getFlag(Flags::V) == 0);
}

// Branch if Overflow Set
void CPU::BVS(int8_t memory)
{
    branch(memory, getFlag(Flags::V) == 1);
}

// Clear Carry
void CPU::CLC()
{
    setFlag(Flags::C, false);
}

// Clear Decimal
void CPU::CLD()
{
    setFlag(Flags::D, false);
}

// Clear Interrupt Disable
void CPU::CLI()
{
    setFlag(Flags::I, false);
}

// Clear Overflow
void CPU::CLV()
{
    setFlag(Flags::V, false);
}

// Compare A
void CPU::CMP(uint8_t memory)
{
    uint16_t result = _A - memory;
    setFlag(Flags::C, _A >= memory);
    setFlag(Flags::Z, _A == memory);
    setFlag(Flags::N, getBit(static_cast<uint8_t>(result), 7));
}

// Compare X
void CPU::CPX(uint8_t memory)
{
    uint16_t result = _X - memory;
    setFlag(Flags::C, _X >= memory);
    setFlag(Flags::Z, _X == memory);
    setFlag(Flags::N, getBit(static_cast<uint8_t>(result), 7));
}

// Compare Y
void CPU::CPY(uint8_t memory)
{
    uint16_t result = _Y - memory;
    setFlag(Flags::C, _Y >= memory);
    setFlag(Flags::Z, _Y == memory);
    setFlag(Flags::N, getBit(static_cast<uint8_t>(result), 7));
}

// Decrement Memory
uint8_t CPU::DEC(uint8_t memory)
{
    uint8_t result = memory - 1;
    setFlag(Flags::Z, result == 0);
    setFlag(Flags::N, getBit(static_cast<uint8_t>(result), 7));
    return result;
}

// Decrement X
void CPU::DEX()
{
    uint8_t result = _X - 1;
    setX(result);
}

// Decrement Y
void CPU::DEY()
{
    uint8_t result = _Y - 1;
    setY(result);
}

// Bitwise Exclusive OR
void CPU::EOR(uint8_t memory)
{
    uint8_t result = _A ^ memory;
    setAccumulator(result);
}

// Increment Memory
uint8_t CPU::INC(uint8_t memory)
{
    uint8_t result = memory + 1;
    setFlag(Flags::Z, result == 0);
    setFlag(Flags::N, getBit(result, 7)); 
    return result;
}

// Increment X
void CPU::INX()
{
    uint8_t result = _X + 1;
    setX(result);
}

//  Increment Y
void CPU::INY()
{
    uint8_t result = _Y + 1;
    setY(result); 
}

// Jump
void CPU::JMP(uint16_t memory)
{
    _pc = memory;
}

// Jump to Subroutine
void CPU::JSR(uint8_t memory)
{
    uint16_t returnAddr = _pc + 1;

    push((returnAddr >> 8) & 0xFF);
    push(returnAddr & 0xFF);

    _pc = memory;
}

// Load A
void CPU::LDA(uint8_t memory)
{
    setAccumulator(memory);
}

// Load X
void CPU::LDX(uint8_t memory)
{
    setX(memory);
}

// Load Y
void CPU::LDY(uint8_t memory)
{
    setY(memory);
}

// Logical Shift Right
uint8_t CPU::LSR(uint8_t value)
{
    uint8_t carry = getBit(value, 0); // value bit 0
    value >>= 1;

    setFlag(Flags::C, carry == 1);
    setFlag(Flags::Z, value == 0);
    setFlag(Flags::N, false); 

    return value;
}

// No Operation
void CPU::NOP()
{}

// Bitwise OR
void CPU::ORA(uint8_t memory)
{
    uint8_t result = _A | memory;
    setAccumulator(result);
}

// Push A
void CPU::PHA()
{
    push(_A);
}

// Push Processor Status
void CPU::PHP()
{
    uint8_t pushedStatus = _status;
    setBit(pushedStatus, static_cast<uint8_t>(Flags::B));
    setBit(pushedStatus, static_cast<uint8_t>(Flags::U));

    push(pushedStatus);
}

// Pull A
void CPU::PLA()
{
    setAccumulator(pull());
}

// Pull Processor Status
void CPU::PLP()
{
    _status = pull();
    clearBit(_status, static_cast<uint8_t>(Flags::B));
    clearBit(_status, static_cast<uint8_t>(Flags::U));
}

// Rotate Right
uint8_t CPU::ROL(uint8_t value)
{
    uint8_t bit7 = getBit(value, 7);

    uint8_t carry = getFlag(Flags::C);
    value <<= 1;
    carry == 1 ? setBit(value, 0) : clearBit(value, 0);

    setFlag(Flags::C, bit7 == 1);
    setFlag(Flags::Z, value == 0);
    setFlag(Flags::N, getBit(value, 7));

    return value;
}

// Rotate Left
uint8_t CPU::ROR(uint8_t value)
{
    uint8_t bit0 = getBit(value, 0);

    uint8_t carry = getFlag(Flags::C);
    value >>= 1;
    carry == 1 ? setBit(value, 7) : clearBit(value, 7);
    
    setFlag(Flags::C, bit0 == 1);
    setFlag(Flags::Z, value == 0);
    setFlag(Flags::N, getBit(value, 7));
    
    return value;
}

// Return from Interrupt
void CPU::RTI()
{
    _status = pull();
    clearBit(_status, static_cast<uint8_t>(Flags::B)); // B flag is not restored
    clearBit(_status, static_cast<uint8_t>(Flags::U)); // U flag is not restored

    uint8_t lo = pull();
    uint8_t hi = pull();
    _pc = (static_cast<uint16_t>(hi) << 8) | lo;
}

// Return from Subroutine
void CPU::RTS()
{
   uint8_t lo = pull();
   uint8_t hi = pull();
   
   _pc = (static_cast<uint16_t>(hi) << 8) | lo; 
   _pc++;
}

// Subtract with Carry
void CPU::SBC(uint8_t memory)
{
    uint16_t diff = _A - memory - (1 - getFlag(Flags::C));
    uint8_t result = static_cast<uint8_t>(diff);

    setFlag(Flags::C, !(diff > 0xFF));
    setFlag(Flags::V, (diff ^ _A) & (diff ^ ~memory) & 0x80);

    setAccumulator(result);
}

// Set Carry
void CPU::SEC()
{
    setFlag(Flags::C, true);
}

// Set Decimal
void CPU::SED()
{
    setFlag(Flags::D, true);
}

// Set Interrupt Disable
void CPU::SEI()
{
    setFlag(Flags::I, true);
}

// Store A
void CPU::STA(uint16_t memory)
{
    _ram.write(memory, _A);
}

// Store X
void CPU::STX(uint16_t memory)
{
    _ram.write(memory, _X);
}

// Store Y
void CPU::STY(uint16_t memory)
{
    _ram.write(memory, _Y);
}

// Transfer A to X
void CPU::TAX()
{
    setX(_A);
}

// Transfer Stack Pointer to X
void CPU::TAY()
{
    setY(_A);
}

// Transfer Stack Pointer to X
void CPU::TSX()
{
    setX(_sp);
}

// Transfer X to A
void CPU::TXA()
{
    setAccumulator(_X);
}
// Transfer X to Stack Pointer
void CPU::TXS()
{
    _sp = _X;
}

// Transfer Y to A
void CPU::TYA()
{
    setAccumulator(_Y);
}