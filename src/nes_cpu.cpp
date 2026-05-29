#include "nes_cpu.h"

using namespace NES;

CPU::CPU(Bus& bus, Region region) : _bus(bus)
{
    CLOCK_FREQUENCY = getClockFrequency(region);
}

// ============================================================
// PUBLIC-FACING METHODS
// ============================================================

void CPU::reset()
{
    _A = 0;
    _X = 0;
    _Y = 0;
    _sp = 0xFD;
    _status = 0x24; // IRQ disabled, unused flag set
    _cycles = 0;

    // Setting the program counter to 0x0000 is for testing purposes only. In a real NES, the reset vector is at 0xFFFC.
    _pc = 0x0000;
}

void CPU::step()
{
    uint8_t opcode = _bus.read(_pc++);
    decodeAndExecute(opcode);
}

void CPU::setPC(uint16_t address)
{
    _pc = address;
}

CPU::State CPU::getState() const
{
    return State{_A, _X, _Y, _pc, _sp, _status, _cycles};
}

// ============================================================
// HELPERS
// ============================================================

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

void CPU::setY(uint8_t value)
{
    setFlag(Flags::Z, value == 0);
    setFlag(Flags::N, getBit(value, 7));
    _Y = value;
}

void CPU::push(uint8_t value)
{
    _bus.write(0x0100 | _sp, value);
    _sp--;
}

uint8_t CPU::pull()
{
    _sp++;
    return _bus.read(0x0100 | _sp);
}

void CPU::branch(int8_t offset, bool condition)
{
    if (condition)
    {
        incrementCycles();
        _pc += offset;
    }
}

// ============================================================
// INSTRUCTIONS
// ============================================================

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

// Break
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

    uint8_t lo = _bus.read(0xFFFE);
    uint8_t hi = _bus.read(0xFFFF);
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
void CPU::JSR(uint16_t memory)
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
{
}

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
    _bus.write(memory, _A);
}

// Store X
void CPU::STX(uint16_t memory)
{
    _bus.write(memory, _X);
}

// Store Y
void CPU::STY(uint16_t memory)
{
    _bus.write(memory, _Y);
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

// ============================================================
// ADDRESSING MODES
// ============================================================

uint8_t CPU::addrAccumulator()
{
    return _A;
}

uint16_t CPU::addrAbsolute()
{
    uint8_t lo = _bus.read(_pc++);
    uint8_t hi = _bus.read(_pc++);

    uint16_t addr = (static_cast<uint16_t>(hi) << 8) | lo;
    return addr;
}

uint16_t CPU::addrAbsoluteX(bool alwaysCrossPage)
{
    uint16_t base = addrAbsolute();
    uint16_t addr = base + _X;
    if (alwaysCrossPage || (base & 0xFF00) != (addr & 0xFF00)) // page boundary crossed
    {
        incrementCycles();
    }
    return addr;
}

uint16_t CPU::addrAbsoluteY(bool alwaysCrossPage)
{
    uint16_t base = addrAbsolute();
    uint16_t addr = base + _Y;
    if (alwaysCrossPage || (base & 0xFF00) != (addr & 0xFF00)) // page boundary crossed
    {
        incrementCycles();
    }
    return addr;
}

// Returns the immediate value (the byte following the opcode)
uint8_t CPU::addrImmediate()
{
    return _bus.read(_pc++);
}

void CPU::addrImplied()
{
    return;
}

uint16_t CPU::addrIndirect()
{
    uint8_t lo = _bus.read(_pc++);
    uint8_t hi = _bus.read(_pc++);
    uint16_t ptr = (static_cast<uint16_t>(hi) << 8) | lo;

    uint8_t addrLo = _bus.read(ptr);
    uint8_t addrHi = (lo == 0xFF) ? _bus.read(ptr & 0xFF00)
                                  : _bus.read(ptr + 1);
    return (static_cast<uint16_t>(addrHi) << 8) | addrLo;
}

uint16_t CPU::addrIndirectX()
{
    uint8_t zp = _bus.read(_pc++);
    uint8_t ptr = zp + _X;

    uint8_t lo = _bus.read(ptr);
    uint8_t hi = _bus.read(ptr + 1);
    return (static_cast<uint16_t>(hi) << 8) | lo;
}

uint16_t CPU::addrIndirectY()
{
    uint8_t zp = _bus.read(_pc++);

    uint8_t lo = _bus.read(zp);
    uint8_t hi = _bus.read(zp + 1);
    uint16_t base = (static_cast<uint16_t>(hi) << 8) | lo;
    uint16_t addr = base + _Y;

    if ((base & 0xFF00) != (addr & 0xFF00)) // page boundary crossed
    {
        incrementCycles();
    }

    return addr;
}

uint16_t CPU::addrZeroPage()
{
    uint8_t zp = _bus.read(_pc++);
    return zp;
}

uint16_t CPU::addrZeroPageX()
{
    uint8_t zp = _bus.read(_pc++);
    return static_cast<uint8_t>(zp + _X);
}

uint16_t CPU::addrZeroPageY()
{
    uint8_t zp = _bus.read(_pc++);
    return static_cast<uint8_t>(zp + _Y);
}

int8_t CPU::addrRelative()
{
    return static_cast<int8_t>(_bus.read(_pc++));
}

// ============================================================
// FETCH-DECODE-EXECUTE
// ============================================================

void CPU::decodeAndExecute(uint8_t opcode)
{
    switch (opcode)
    {
    // Add with Carry
    case 0x69:
    {
        ADC(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0x65:
    {
        ADC(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0x75:
    {
        ADC(_bus.read(addrZeroPageX()));
        incrementCycles(4);
        break;
    }
    case 0x6D:
    {
        ADC(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }
    case 0x7D:
    {
        ADC(_bus.read(addrAbsoluteX()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0x79:
    {
        ADC(_bus.read(addrAbsoluteY()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0x61:
    {
        ADC(_bus.read(addrIndirectX()));
        incrementCycles(6);
        break;
    }
    case 0x71:
    {
        ADC(_bus.read(addrIndirectY()));
        incrementCycles(5); // +1 if page boundary crossed
        break;
    }

    // Bitwise AND
    case 0x29:
    {
        AND(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0x25:
    {
        AND(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0x35:
    {
        AND(_bus.read(addrZeroPageX()));
        incrementCycles(4);
        break;
    }
    case 0x2D:
    {
        AND(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }
    case 0x3D:
    {
        AND(_bus.read(addrAbsoluteX()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0x39:
    {
        AND(_bus.read(addrAbsoluteY()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0x21:
    {
        AND(_bus.read(addrIndirectX()));
        incrementCycles(6);
        break;
    }
    case 0x31:
    {
        AND(_bus.read(addrIndirectY()));
        incrementCycles(5); // +1 if page boundary crossed
        break;
    }

    // Arithmetic Shift Left
    case 0x0A:
    {
        uint8_t value = addrAccumulator();
        uint8_t result = ASL(value);
        _A = result;
        incrementCycles(2);
        break;
    }
    case 0x06:
    {
        uint16_t addr = addrZeroPage();
        uint8_t value = _bus.read(addr);
        uint8_t result = ASL(value);
        _bus.write(addr, result);
        incrementCycles(5);
        break;
    }
    case 0x16:
    {
        uint16_t addr = addrZeroPageX();
        uint8_t value = _bus.read(addr);
        uint8_t result = ASL(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }
    case 0x0E:
    {
        uint16_t addr = addrAbsolute();
        uint8_t value = _bus.read(addr);
        uint8_t result = ASL(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }
    case 0x1E:
    {
        uint16_t addr = addrAbsoluteX(true); // always add cycle for page boundary check
        uint8_t value = _bus.read(addr);
        uint8_t result = ASL(value);
        _bus.write(addr, result);
        incrementCycles(7);
        break;
    }

    // Branch if Carry Clear
    case 0x90:
    {
        BCC(addrRelative());
        incrementCycles(2); // +1 if branch taken, +2 if page boundary crossed
        break;
    }

    // Branch if Carry Set
    case 0xB0:
    {
        uint16_t prevPc = _pc + 1; // store for page boundary check
        BCS(addrRelative());
        if ((prevPc & 0xFF00) != (_pc & 0xFF00)) // page boundary crossed
            incrementCycles();
        incrementCycles(2); // +1 if branch taken, +2 if page boundary crossed
        break;
    }

    // Branch if Equal
    case 0xF0:
    {
        uint16_t prevPc = _pc + 1; // store for page boundary check
        BEQ(addrRelative());
        if ((prevPc & 0xFF00) != (_pc & 0xFF00)) // page boundary crossed
            incrementCycles();
        incrementCycles(2); // +1 if branch taken, +2 if page boundary crossed
        break;
    }

    // Bit Test
    case 0x24:
    {
        BIT(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0x2C:
    {
        BIT(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }

    // Branch if Minus
    case 0x30:
    {
        uint16_t prevPc = _pc + 1; // store for page boundary check
        BMI(addrRelative());
        if ((prevPc & 0xFF00) != (_pc & 0xFF00)) // page boundary crossed
            incrementCycles();
        incrementCycles(2); // +1 if branch taken, +2 if page boundary crossed
        break;
    }

    // Branch if Not Equal
    case 0xD0:
    {
        uint16_t prevPc = _pc + 1; // store for page boundary check
        BNE(addrRelative());
        if ((prevPc & 0xFF00) != (_pc & 0xFF00)) // page boundary crossed
            incrementCycles();
        incrementCycles(2); // +1 if branch taken, +2 if page boundary crossed
        break;
    }

    // Branch if Plus
    case 0x10:
    {
        uint16_t prevPc = _pc + 1; // store for page boundary check
        BPL(addrRelative());
        if ((prevPc & 0xFF00) != (_pc & 0xFF00)) // page boundary crossed
            incrementCycles();
        incrementCycles(2); // +1 if branch taken, +2 if page boundary crossed
        break;
    }

    // Break
    case 0x00:
    {
        BRK();
        incrementCycles(7);
        break;
    }

    // Branch if Overflow Clear
    case 0x50:
    {
        uint16_t prevPc = _pc + 1; // store for page boundary check
        BVC(addrRelative());
        if ((prevPc & 0xFF00) != (_pc & 0xFF00)) // page boundary crossed
            incrementCycles();
        incrementCycles(2); // +1 if branch taken, +2 if page boundary crossed
        break;
    }

    // Branch if Overflow Set
    case 0x70:
    {
        uint16_t prevPc = _pc + 1; // store for page boundary check
        BVS(addrRelative());
        if ((prevPc & 0xFF00) != (_pc & 0xFF00)) // page boundary crossed
            incrementCycles();
        incrementCycles(2); // +1 if branch taken, +2 if page boundary crossed
        break;
    }

    // Clear Carry
    case 0x18:
    {
        CLC();
        incrementCycles(2);
        break;
    }

    // Clear Decimal
    case 0xD8:
    {
        CLD();
        incrementCycles(2);
        break;
    }

    // Clear Interrup Disable
    case 0x58:
    {
        CLI();
        incrementCycles(2);
        break;
    }

    // Clear Overflow
    case 0xB8:
    {
        CLV();
        incrementCycles(2);
        break;
    }

    // Compare A
    case 0xC9:
    {
        CMP(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0xC5:
    {
        CMP(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0xD5:
    {
        CMP(_bus.read(addrZeroPageX()));
        incrementCycles(4);
        break;
    }
    case 0xCD:
    {
        CMP(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }
    case 0xDD:
    {
        CMP(_bus.read(addrAbsoluteX()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0xD9:
    {
        CMP(_bus.read(addrAbsoluteY()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0xC1:
    {
        CMP(_bus.read(addrIndirectX()));
        incrementCycles(6);
        break;
    }
    case 0xD1:
    {
        CMP(_bus.read(addrIndirectY()));
        incrementCycles(5); // +1 if page boundary crossed
        break;
    }

    // Compare X
    case 0xE0:
    {
        CPX(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0xE4:
    {
        CPX(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0xEC:
    {
        CPX(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }

    // Compare Y
    case 0xC0:
    {
        CPY(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0xC4:
    {
        CPY(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0xCC:
    {
        CPY(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }

    // Decrement Memory
    case 0xC6:
    {
        uint16_t addr = addrZeroPage();
        uint8_t value = _bus.read(addr);
        uint8_t result = DEC(value);
        _bus.write(addr, result);
        incrementCycles(5);
        break;
    }
    case 0xD6:
    {
        uint16_t addr = addrZeroPageX();
        uint8_t value = _bus.read(addr);
        uint8_t result = DEC(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }
    case 0xCE:
    {
        uint16_t addr = addrAbsolute();
        uint8_t value = _bus.read(addr);
        uint8_t result = DEC(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }
    case 0xDE:
    {
        uint16_t addr = addrAbsoluteX(true); // always add cycle for page boundary check
        uint8_t value = _bus.read(addr);
        uint8_t result = DEC(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }

    // Decrement X
    case 0xCA:
    {
        DEX();
        incrementCycles(2);
        break;
    }

    // Decrement Y
    case 0x88:
    {
        DEY();
        incrementCycles(2);
        break;
    }

    // Bitwise Exclusive OR
    case 0x49:
    {
        EOR(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0x45:
    {
        EOR(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0x55:
    {
        EOR(_bus.read(addrZeroPageX()));
        incrementCycles(4);
        break;
    }
    case 0x4D:
    {
        EOR(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }
    case 0x5D:
    {
        EOR(_bus.read(addrAbsoluteX()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0x59:
    {
        EOR(_bus.read(addrAbsoluteY()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0x41:
    {
        EOR(_bus.read(addrIndirectX()));
        incrementCycles(6);
        break;
    }
    case 0x51:
    {
        EOR(_bus.read(addrIndirectY()));
        incrementCycles(5); // +1 if page boundary crossed
        break;
    }

    // Increment Memory
    case 0xE6:
    {
        uint16_t addr = addrZeroPage();
        uint8_t value = _bus.read(addr);
        uint8_t result = INC(value);
        _bus.write(addr, result);
        incrementCycles(5);
        break;
    }
    case 0xF6:
    {
        uint16_t addr = addrZeroPageX();
        uint8_t value = _bus.read(addr);
        uint8_t result = INC(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }
    case 0xEE:
    {
        uint16_t addr = addrAbsolute();
        uint8_t value = _bus.read(addr);
        uint8_t result = INC(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }
    case 0xFE:
    {
        uint16_t addr = addrAbsoluteX(true); // always add cycle for page boundary check
        uint8_t value = _bus.read(addr);
        uint8_t result = INC(value);
        _bus.write(addr, result);
        incrementCycles(7);
        break;
    }

    // Increment X
    case 0xE8:
    {
        INX();
        incrementCycles(2);
        break;
    }

    // Increment Y
    case 0xC8:
    {
        INY();
        incrementCycles(2);
        break;
    }

    // Jump
    case 0x4C:
    {
        JMP(addrAbsolute());
        incrementCycles(3);
        break;
    }

    // Jump Indirect
    case 0x6C:
    {
        JMP(addrIndirect());
        incrementCycles(5);
        break;
    }

    // Jump to Subroutine
    case 0x20:
    {
        JSR(addrAbsolute());
        incrementCycles(6);
        break;
    }

    // Load A
    case 0xA9:
    {
        LDA(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0xA5:
    {
        LDA(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0xB5:
    {
        LDA(_bus.read(addrZeroPageX()));
        incrementCycles(4);
        break;
    }
    case 0xAD:
    {
        LDA(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }
    case 0xBD:
    {
        LDA(_bus.read(addrAbsoluteX()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0xB9:
    {
        LDA(_bus.read(addrAbsoluteY()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0xA1:
    {
        LDA(_bus.read(addrIndirectX()));
        incrementCycles(6);
        break;
    }
    case 0xB1:
    {
        LDA(_bus.read(addrIndirectY()));
        incrementCycles(5); // +1 if page boundary crossed
        break;
    }

    // Load X
    case 0xA2:
    {
        LDX(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0xA6:
    {
        LDX(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0xB6:
    {
        LDX(_bus.read(addrZeroPageY()));
        incrementCycles(4);
        break;
    }
    case 0xAE:
    {
        LDX(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }
    case 0xBE:
    {
        LDX(_bus.read(addrAbsoluteY()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }

    // Load Y
    case 0xA0:
    {
        LDY(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0xA4:
    {
        LDY(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0xB4:
    {
        LDY(_bus.read(addrZeroPageX()));
        incrementCycles(4);
        break;
    }
    case 0xAC:
    {
        LDY(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }
    case 0xBC:
    {
        LDY(_bus.read(addrAbsoluteX()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }

    // Logical Shift Right
    case 0x4A:
    {
        uint8_t value = addrAccumulator();
        uint8_t result = LSR(value);
        _A = result;
        incrementCycles(2);
        break;
    }
    case 0x46:
    {
        uint16_t addr = addrZeroPage();
        uint8_t value = _bus.read(addr);
        uint8_t result = LSR(value);
        _bus.write(addr, result);
        incrementCycles(5);
        break;
    }
    case 0x56:
    {
        uint16_t addr = addrZeroPageX();
        uint8_t value = _bus.read(addr);
        uint8_t result = LSR(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }
    case 0x4E:
    {
        uint16_t addr = addrAbsolute();
        uint8_t value = _bus.read(addr);
        uint8_t result = LSR(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }
    case 0x5E:
    {
        uint16_t addr = addrAbsoluteX(true); // always add cycle for page boundary check
        uint8_t value = _bus.read(addr);
        uint8_t result = LSR(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }

    // No Operation
    case 0xEA:
    {
        NOP();
        incrementCycles(2);
        break;
    }

    // Bitwise OR
    case 0x09:
    {
        ORA(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0x05:
    {
        ORA(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0x15:
    {
        ORA(_bus.read(addrZeroPageX()));
        incrementCycles(4);
        break;
    }
    case 0x0D:
    {
        ORA(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }
    case 0x1D:
    {
        ORA(_bus.read(addrAbsoluteX()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0x19:
    {
        ORA(_bus.read(addrAbsoluteY()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0x01:
    {
        ORA(_bus.read(addrIndirectX()));
        incrementCycles(6);
        break;
    }
    case 0x11:
    {
        ORA(_bus.read(addrIndirectY()));
        incrementCycles(5); // +1 if page boundary crossed
        break;
    }

    // Push A
    case 0x48:
    {
        PHA();
        incrementCycles(3);
        break;
    }

    // Push Processor Status
    case 0x08:
    {
        PHP();
        incrementCycles(3);
        break;
    }

    // Pull Processor Status
    case 0x28:
    {
        PLP();
        incrementCycles(4);
        break;
    }

    // Rotate Left
    case 0x2A:
    {
        uint8_t value = addrAccumulator();
        uint8_t result = ROL(value);
        _A = result;
        incrementCycles(2);
        break;
    }
    case 0x26:
    {
        uint16_t addr = addrZeroPage();
        uint8_t value = _bus.read(addr);
        uint8_t result = ROL(value);
        _bus.write(addr, result);
        incrementCycles(5);
        break;
    }
    case 0x36:
    {
        uint16_t addr = addrZeroPageX();
        uint8_t value = _bus.read(addr);
        uint8_t result = ROL(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }
    case 0x2E:
    {
        uint16_t addr = addrAbsolute();
        uint8_t value = _bus.read(addr);
        uint8_t result = ROL(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }
    case 0x3E:
    {
        uint16_t addr = addrAbsoluteX(true); // always add cycle for page boundary check
        uint8_t value = _bus.read(addr);
        uint8_t result = ROL(value);
        _bus.write(addr, result);
        incrementCycles(6);
        break;
    }

    // Rotate Right
    case 0x6A:
    {
        uint8_t value = addrAccumulator();
        uint8_t result = ROR(value);
        _A = result;
        incrementCycles(2);
        break;
    }

    // Return from Interrupt
    case 0x40:
    {
        RTI();
        incrementCycles(6);
        break;
    }

    // Return from Subroutine
    case 0x60:
    {
        RTS();
        incrementCycles(6);
        break;
    }

    // Subtract with Carry
    case 0xE9:
    {
        SBC(addrImmediate());
        incrementCycles(2);
        break;
    }
    case 0xE5:
    {
        SBC(_bus.read(addrZeroPage()));
        incrementCycles(3);
        break;
    }
    case 0xF5:
    {
        SBC(_bus.read(addrZeroPageX()));
        incrementCycles(4);
        break;
    }
    case 0xED:
    {
        SBC(_bus.read(addrAbsolute()));
        incrementCycles(4);
        break;
    }
    case 0xFD:
    {
        SBC(_bus.read(addrAbsoluteX()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0xF9:
    {
        SBC(_bus.read(addrAbsoluteY()));
        incrementCycles(4); // +1 if page boundary crossed
        break;
    }
    case 0xE1:
    {
        SBC(_bus.read(addrIndirectX()));
        incrementCycles(6);
        break;
    }
    case 0xF1:
    {
        SBC(_bus.read(addrIndirectY()));
        incrementCycles(5); // +1 if page boundary crossed
        break;
    }

    // Set Carry
    case 0x38:
    {
        SEC();
        incrementCycles(2);
        break;
    }

    // Set Decimal
    case 0xF8:
    {
        SED();
        incrementCycles(2);
        break;
    }

    // Set Interrupt Disable
    case 0x78:
    {
        SEI();
        incrementCycles(2);
        break;
    }

    // Store A
    case 0x85:
    {
        STA(addrZeroPage());
        incrementCycles(3);
        break;
    }
    case 0x95:
    {
        STA(addrZeroPageX());
        incrementCycles(4);
        break;
    }
    case 0x8D:
    {
        STA(addrAbsolute());
        incrementCycles(4);
        break;
    }
    case 0x9D:
    {
        STA(addrAbsoluteX(true)); // always add cycle for page boundary check
        incrementCycles(4);       // +1 if page boundary crossed
        break;
    }
    case 0x99:
    {
        STA(addrAbsoluteY(true)); // always add cycle for page boundary check
        incrementCycles(4);       // +1 if page boundary crossed
        break;
    }
    case 0x81:
    {
        STA(addrIndirectX());
        incrementCycles(6);
        break;
    }
    case 0x91:
    {
        STA(addrIndirectY());
        incrementCycles(6);
        break;
    }

    // Store X
    case 0x86:
    {
        STX(addrZeroPage());
        incrementCycles(3);
        break;
    }
    case 0x96:
    {
        STX(addrZeroPageY());
        incrementCycles(4);
        break;
    }
    case 0x8E:
    {
        STX(addrAbsolute());
        incrementCycles(4);
        break;
    }

    // Store Y
    case 0x84:
    {
        STY(addrZeroPage());
        incrementCycles(3);
        break;
    }
    case 0x94:
    {
        STY(addrZeroPageX());
        incrementCycles(4);
        break;
    }
    case 0x8C:
    {
        STY(addrAbsolute());
        incrementCycles(4);
        break;
    }

    // Transfer A to X
    case 0xAA:
    {
        TAX();
        incrementCycles(2);
        break;
    }

    // Transfer A to Y
    case 0xA8:
    {
        TAY();
        incrementCycles(2);
        break;
    }

    // Transfer Stack Pointer to X
    case 0xBA:
    {
        TSX();
        incrementCycles(2);
        break;
    }

    // Transfer X to A
    case 0x8A:
    {
        TXA();
        incrementCycles(2);
        break;
    }

    // Transfer X to Stack Pointer
    case 0x9A:
    {
        TXS();
        incrementCycles(2);
        break;
    }

    // Transfer Y to A
    case 0x98:
    {
        TYA();
        incrementCycles(2);
        break;
    }
    }
}