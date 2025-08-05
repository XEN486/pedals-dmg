#include "cpu.h"
#include <print>

using namespace dmg::cpu;

void SM83::LD(uint16_t& dst, uint16_t src, uint8_t cycles) {
	dst = src;
	m_LastOpCycles += cycles;
}

void SM83::LD(uint8_t& dst, uint8_t src, uint8_t cycles) {
	dst = src;
	m_LastOpCycles += cycles;
}

void SM83::LD(uint16_t addr, uint8_t src, uint8_t cycles) {
	m_Bus->WriteMemory(addr, src);
	m_LastOpCycles += cycles;
}

void SM83::LD(uint8_t& dst, uint16_t addr, uint8_t cycles) {
	dst = m_Bus->ReadMemory(addr);
	m_LastOpCycles += cycles;
}

void SM83::LD(uint16_t& dst, uint16_t src, int8_t n, uint8_t cycles) {
	int result = static_cast<int>(src + n);
	
	ClearFlag(Flags::Zero);
	ClearFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((m_Registers.sp ^ n ^ (result & 0xffff)) & 0x10) == 0x10);
	SetFlagByValue(Flags::Carry, ((m_Registers.sp ^ n ^ (result & 0xffff)) & 0x100) == 0x100);
	
	dst = static_cast<uint16_t>(result);
	m_LastOpCycles += cycles;
}

void SM83::LD_addr(uint16_t addr, uint16_t src, uint8_t cycles) {
	m_Bus->WriteMemory16(addr, src);
	m_LastOpCycles += cycles;
}

void SM83::INC(uint8_t& reg) {
	uint8_t result = reg + 1;
	SetFlagByValue(Flags::Zero, result == 0);
	SetFlagByValue(Flags::HalfCarry, ((reg & 0xf) + 1) > 0xf);
	ClearFlag(Flags::Subtraction);

	reg = result;
	m_LastOpCycles += 4;
}

void SM83::INC(uint16_t& reg) {
	reg++;
	m_LastOpCycles += 8;
}

void SM83::INC_addr(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	uint8_t result = val + 1;

	SetFlagByValue(Flags::Zero, result == 0);
	SetFlagByValue(Flags::HalfCarry, ((val & 0xf) + 1) > 0xf);
	ClearFlag(Flags::Subtraction);

	m_Bus->WriteMemory(addr, result);
	m_LastOpCycles += 12;
}

void SM83::DEC(uint8_t& reg) {
	uint8_t result = reg - 1;

	SetFlagByValue(Flags::Zero, result == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, (result & 0xf) == 0xf);
	
	reg = result;
	m_LastOpCycles += 4;
}

void SM83::DEC(uint16_t& reg) {
	reg--;
	m_LastOpCycles += 8;
}

void SM83::DEC_addr(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	uint8_t result = val - 1;
	
	SetFlagByValue(Flags::Zero, result == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, (result & 0xf) == 0xf);

	m_Bus->WriteMemory(addr, result);
	m_LastOpCycles += 12;
}

void SM83::XOR(uint8_t& reg, uint8_t src, uint8_t cycles) {
	reg ^= src;
	
	SetFlagByValue(Flags::Zero, reg == 0);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Carry);
	
	m_LastOpCycles += cycles;
}

void SM83::XOR(uint8_t& reg, uint16_t addr, uint8_t cycles) {
	uint8_t val = m_Bus->ReadMemory(addr);
	reg ^= val;
	
	SetFlagByValue(Flags::Zero, reg == 0);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Carry);
	
	m_LastOpCycles += cycles;
}

void SM83::JR(int8_t rel) {
	m_Registers.pc += rel;
	m_LastOpCycles += 12;
}

void SM83::JR(Flags flag, bool inverse, int8_t rel) {
	m_LastOpCycles += 8;
	if ((m_Registers.f & flag) == (inverse ? 0 : flag)) {
		m_Registers.pc += rel;
		m_LastOpCycles += 4;
	}
}

void SM83::CALL(uint16_t addr) {
	StackPush16(m_Registers.pc);

	m_Registers.pc = addr;
	m_LastOpCycles += 24;
}

void SM83::CALL(Flags flag, bool inverse, uint16_t addr) {
	m_LastOpCycles += 12;

	if ((m_Registers.f & flag) == (inverse ? 0 : flag)) {
		StackPush16(m_Registers.pc);
		m_Registers.pc = addr;

		m_LastOpCycles += 12;
	}
}

void SM83::PUSH(uint16_t reg) {
	StackPush16(reg);
	m_LastOpCycles += 16;
}

void SM83::RLA() {
	bool new_carry = (m_Registers.a >> 7) & 1;
	m_Registers.a = (m_Registers.a << 1) | static_cast<uint8_t>(GetFlag(Flags::Carry));
	
	SetFlagByValue(Flags::Carry, new_carry);
	ClearFlag(Flags::Zero);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);

	m_LastOpCycles += 4;
}


void SM83::POP(uint16_t& reg) {
	reg = StackPop16();
	m_LastOpCycles += 12;
}

void SM83::POP_af() {
	uint16_t popped = StackPop16();
	m_Registers.a = (popped >> 8) & 0xff;
	m_Registers.f = popped & 0xf0;

	m_LastOpCycles += 12;
}

void SM83::RET() {
	m_Registers.pc = StackPop16();
	m_LastOpCycles += 16;
}

void SM83::RET(Flags flag, bool inverse) {
	m_LastOpCycles += 8;

	if ((m_Registers.f & flag) == (inverse ? 0 : flag)) {
		m_Registers.pc = StackPop16();

		m_LastOpCycles += 12;
	}
}

void SM83::CP(uint8_t value, uint8_t cycles) {
	uint8_t result = value - m_Registers.a;

	SetFlagByValue(Flags::Zero, result == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, (m_Registers.a & 0xf) < (value & 0xf));
	SetFlagByValue(Flags::Carry, value > m_Registers.a);

	m_LastOpCycles += cycles;
}

void SM83::CP(uint16_t addr, uint8_t cycles) {
	uint8_t value = m_Bus->ReadMemory(addr);
	uint8_t result = value - m_Registers.a;
	
	SetFlagByValue(Flags::Zero, result == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, (m_Registers.a & 0xf) < (value & 0xf));
	SetFlagByValue(Flags::Carry, m_Registers.a < value);

	m_LastOpCycles += cycles;
}

void SM83::JP(uint16_t addr, uint8_t cycles) {
	m_Registers.pc = addr;
	m_LastOpCycles += cycles;
}

void SM83::JP(Flags flag, bool inverse, uint16_t addr) {
	m_LastOpCycles += 12;

	if ((m_Registers.f & flag) == (inverse ? 0 : flag)) {
		m_Registers.pc = addr;
		m_LastOpCycles += 4;
	}
}

void SM83::DI() {
	m_IME = false;
	m_LastOpCycles += 4;
}

void SM83::EI() {
	m_EIqueued = true;
	m_LastOpCycles += 4;
}

void SM83::RETI() {
	m_IME = true;
	m_Registers.pc = StackPop16();
	m_LastOpCycles += 16;
}

void SM83::SUB(uint8_t value, uint8_t cycles) {
	uint8_t reg = m_Registers.a;
	m_Registers.a -= value;

	SetFlagByValue(Flags::Zero, m_Registers.a == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((reg & 0xf) - (value & 0xf)) < 0);
	SetFlagByValue(Flags::Carry, reg < value);

	m_LastOpCycles += cycles;
}

void SM83::SUB(uint16_t addr) {
	uint8_t reg = m_Registers.a;
	uint8_t value = m_Bus->ReadMemory(addr);
	m_Registers.a -= value;

	SetFlagByValue(Flags::Zero, m_Registers.a == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((reg & 0xf) - (value & 0xf)) < 0);
	SetFlagByValue(Flags::Carry, reg < value);

	m_LastOpCycles += 8;
}

void SM83::ADD(uint8_t value, uint8_t cycles) {
	uint8_t reg = m_Registers.a;
	uint16_t result = value + m_Registers.a;
	m_Registers.a = static_cast<uint8_t>(result);

	SetFlagByValue(Flags::Zero, m_Registers.a == 0);
	ClearFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((reg & 0xf) + (value & 0xf)) > 0xf);
	SetFlagByValue(Flags::Carry, (result & 0x100) != 0);

	m_LastOpCycles += cycles;
}

void SM83::ADD(uint16_t addr) {
	uint8_t reg = m_Registers.a;
	uint8_t value = m_Bus->ReadMemory(addr);
	uint16_t result = value + m_Registers.a;
	m_Registers.a = static_cast<uint8_t>(result);

	SetFlagByValue(Flags::Zero, m_Registers.a == 0);
	ClearFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((reg & 0xf) + (value & 0xf)) > 0xf);
	SetFlagByValue(Flags::Carry, result > 0xFF);

	m_LastOpCycles += 8;
}

void SM83::ADDhl(uint16_t reg) {
	uint16_t r = m_Registers.hl;
	size_t result = m_Registers.hl + reg;

	ClearFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((r & 0xfff) + (reg & 0xfff)) > 0xfff);
	SetFlagByValue(Flags::Carry, (result & 0x10000) != 0);
	m_Registers.hl = static_cast<uint16_t>(result);

	m_LastOpCycles += 8;
}

void SM83::ADD(int8_t value) {
	int result = static_cast<int>(m_Registers.sp + value);

	ClearFlag(Flags::Zero);
	ClearFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((m_Registers.sp ^ value ^ (result & 0xffff)) & 0x10) == 0x10);
	SetFlagByValue(Flags::Carry, ((m_Registers.sp ^ value ^ (result & 0xffff)) & 0x100) == 0x100);

	m_Registers.sp = static_cast<uint16_t>(result);
	m_LastOpCycles += 16;
}

void SM83::OR(uint8_t src, uint8_t cycles) {
	m_Registers.a |= src;

	SetFlagByValue(Flags::Zero, m_Registers.a == 0);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Carry);

	m_LastOpCycles += cycles;
}

void SM83::OR(uint16_t addr) {
	m_Registers.a |= m_Bus->ReadMemory(addr);
	
	SetFlagByValue(Flags::Zero, m_Registers.a == 0);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Carry);

	m_LastOpCycles += 8;
}

void SM83::CPL() {
	m_Registers.a = ~m_Registers.a;
	SetFlag(Flags::Subtraction);
	SetFlag(Flags::HalfCarry);

	m_LastOpCycles += 4;
}

void SM83::AND(uint8_t src, uint8_t cycles) {
	m_Registers.a &= src;
	SetFlagByValue(Flags::Zero, m_Registers.a == 0);
	SetFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::Carry);

	m_LastOpCycles += cycles;
}

void SM83::AND(uint16_t addr) {
	m_Registers.a &= m_Bus->ReadMemory(addr);
	SetFlagByValue(Flags::Zero, m_Registers.a == 0);
	SetFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::Carry);

	m_LastOpCycles += 8;
}

void SM83::RST(uint8_t vec) {
	StackPush16(m_Registers.pc);

	m_Registers.pc = vec;
	m_LastOpCycles += 16;
}

void SM83::ADC(uint8_t value, uint8_t cycles) {
	uint8_t reg = m_Registers.a;
	uint16_t result = value + m_Registers.a + GetFlag(Flags::Carry);
	m_Registers.a = static_cast<uint8_t>(result);

	SetFlagByValue(Flags::Zero, m_Registers.a == 0);
	ClearFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((reg & 0xf) + (value & 0xf) + GetFlag(Flags::Carry)) > 0xf);
	SetFlagByValue(Flags::Carry, (result & 0x100) != 0);

	m_LastOpCycles += cycles;
}

void SM83::HALT() {
	m_State = State::Halt;
	if (!m_IME) {
		m_DontExecuteHandler = true;
		m_DoubleRead = (m_Bus->GetIE() & m_Bus->GetIF() & 0x1f) != 0;
	}

	m_LastOpCycles += 4;
}

void SM83::RLCA() {
	uint8_t carry = (m_Registers.a >> 7) & 1;
	m_Registers.a = (m_Registers.a << 1) | carry;

	SetFlagByValue(Flags::Carry, carry);
	ClearFlag(Flags::Zero);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);

	m_LastOpCycles += 4;
}

// https://github.com/LIJI32/SameBoy/blob/master/Core/sm83_cpu.c
void SM83::DAA() {
	int16_t result = m_Registers.af >> 8;

	m_Registers.af &= ~(0xFF00 | Flags::Zero);

	if (m_Registers.af & Flags::Subtraction) {
		if (m_Registers.af & Flags::HalfCarry) {
			result = (result - 0x06) & 0xFF;
		}

		if (m_Registers.af & Flags::Carry) {
			result -= 0x60;
		}
	}

	else {
		if ((m_Registers.af & Flags::HalfCarry) || (result & 0x0F) > 0x09) {
			result += 0x06;
		}

		if ((m_Registers.af & Flags::Carry) || result > 0x9F) {
			result += 0x60;
		}
	}

	if ((result & 0xFF) == 0) {
		m_Registers.af |= Flags::Zero;
	}

	if ((result & 0x100) == 0x100) {
		m_Registers.af |= Flags::Carry;
	}

	m_Registers.af &= ~Flags::HalfCarry;
	m_Registers.af |= result << 8;

	m_LastOpCycles += 4;
}

void SM83::RRA() {
	bool lsb = m_Registers.a & 1;
	uint8_t carry = GetFlag(Flags::Carry) ? 0x80 : 0;
	m_Registers.a = (m_Registers.a >> 1) | carry;

	SetFlagByValue(Flags::Carry, lsb);
	ClearFlag(Flags::Zero);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);

	m_LastOpCycles += 4;
}

void SM83::CCF() {
	SetFlagByValue(Flags::Carry, !GetFlag(Flags::Carry));
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);

	m_LastOpCycles += 4;
}

void SM83::STOP() {
	m_State = State::Stop;

	m_LastOpCycles += 4;
}

void SM83::SCF() {
	SetFlag(Flags::Carry);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 4;
}

void SM83::SBC(uint8_t value, uint8_t cycles) {
	uint8_t reg = m_Registers.a;
	int result = m_Registers.a - value - GetFlag(Flags::Carry);

	m_Registers.a = static_cast<uint8_t>(result);

	SetFlagByValue(Flags::Zero, m_Registers.a == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((reg & 0xf) - (value & 0xf) - GetFlag(Flags::Carry)) < 0);
	SetFlagByValue(Flags::Carry, result < 0);

	m_LastOpCycles += cycles;
}

void SM83::RRCA() {
	uint8_t carry = m_Registers.a & 1;
	m_Registers.a = ((m_Registers.a >> 1) | (carry << 7));

	SetFlagByValue(Flags::Carry, carry);
	ClearFlag(Flags::Zero);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 4;
}

void SM83::BIT(uint8_t bit, uint8_t reg) {
	m_Registers.af &= 0xff00 | Flags::Carry;
	m_Registers.af |= Flags::HalfCarry;

	ClearFlag(Flags::Subtraction);

	if (!((1 << bit) & reg)) {
		m_Registers.af |= Flags::Zero;
	}

	m_LastOpCycles += 8;
}

void SM83::BIT(uint8_t bit, uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	
	SetFlagByValue(Flags::Zero, (val & (1 << bit)) == 0);
	ClearFlag(Flags::Subtraction);
	SetFlag(Flags::HalfCarry);

	m_LastOpCycles += 12;
}

void SM83::RL(uint8_t& reg) {
	uint8_t carry = (m_Registers.f & Flags::Carry) ? 1 : 0;
	SetFlagByValue(Flags::Carry, (reg >> 7) & 1);

	reg <<= 1;
	reg |= carry;

	SetFlagByValue(Flags::Zero, reg == 0);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);

	m_LastOpCycles += 8;
}

void SM83::RL(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	uint8_t carry = (m_Registers.f & Flags::Carry) ? 1 : 0;
	SetFlagByValue(Flags::Carry, (val >> 7) & 1);

	val <<= 1;
	val |= carry;

	m_Bus->WriteMemory(addr, val);
	SetFlagByValue(Flags::Zero, val == 0);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);

	m_LastOpCycles += 16;
}

void SM83::SWAP(uint8_t& reg) {
	reg = (reg << 4) | (reg >> 4);

	SetFlagByValue(Flags::Zero, reg == 0);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Carry);

	m_LastOpCycles += 8;
}

void SM83::SWAP(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	val = (val << 4) | (val >> 4);

	m_Bus->WriteMemory(addr, val);

	SetFlagByValue(Flags::Zero, val == 0);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Carry);

	m_LastOpCycles += 16;
}

void SM83::RES(uint8_t u3, uint8_t& reg) {
	reg &= ~(1 << u3);
	
	m_LastOpCycles += 8;
}

void SM83::RES(uint8_t u3, uint16_t addr) {
	m_Bus->WriteMemory(addr, m_Bus->ReadMemory(addr) & ~(1 << u3));

	m_LastOpCycles += 16;
}

void SM83::SET(uint8_t u3, uint8_t& reg) {
	reg |= (1 << u3);
	
	m_LastOpCycles += 8;
}

void SM83::SET(uint8_t u3, uint16_t addr) {
	m_Bus->WriteMemory(addr, m_Bus->ReadMemory(addr) | (1 << u3));

	m_LastOpCycles += 16;
}

void SM83::SLA(uint8_t& reg) {
	bool carry = reg & 0x80;
	reg <<= 1;

	SetFlagByValue(Flags::Zero, reg == 0);
	SetFlagByValue(Flags::Carry, carry);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 8;
}

void SM83::SLA(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	bool carry = val & 0x80;

	val <<= 1;
	m_Bus->WriteMemory(addr, val);

	SetFlagByValue(Flags::Zero, val == 0);
	SetFlagByValue(Flags::Carry, carry);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 16;
}

void SM83::RLC(uint8_t& reg) {
	uint8_t carry = (reg & 0x80) >> 7;
	reg = (reg << 1) | carry;

	SetFlagByValue(Flags::Carry, carry);
	SetFlagByValue(Flags::Zero, reg == 0);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 8;
}

void SM83::RLC(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	uint8_t carry = (val & 0x80) >> 7;

	val = (val << 1) | carry;
	m_Bus->WriteMemory(addr, val);

	SetFlagByValue(Flags::Carry, carry);
	SetFlagByValue(Flags::Zero, val == 0);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 16;
}

void SM83::SRL(uint8_t& reg) {
	bool lsb = reg & 1;
	reg >>= 1;

	SetFlagByValue(Flags::Carry, lsb);
	SetFlagByValue(Flags::Zero, reg == 0);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 8;
}

void SM83::SRL(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	bool lsb = val & 1;

	val >>= 1;
	m_Bus->WriteMemory(addr, val);

	SetFlagByValue(Flags::Carry, lsb);
	SetFlagByValue(Flags::Zero, val == 0);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 16;
}

void SM83::RR(uint8_t& reg) {
	uint8_t carry = GetFlag(Flags::Carry);

	bool lsb = reg & 1;
	SetFlagByValue(Flags::Carry, lsb);

	reg >>= 1;
	reg |= (carry << 7);

	SetFlagByValue(Flags::Zero, reg == 0);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);

	m_LastOpCycles += 8;
}

void SM83::RR(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	uint8_t carry = GetFlag(Flags::Carry);

	bool lsb = val & 1;
	SetFlagByValue(Flags::Carry, lsb);

	val >>= 1;
	val |= (carry << 7);
	m_Bus->WriteMemory(addr, val);

	SetFlagByValue(Flags::Zero, val == 0);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);

	m_LastOpCycles += 16;
}

void SM83::RRC(uint8_t& reg) {
	uint8_t carry = reg & 1;
	reg = ((reg >> 1) | (carry << 7));

	SetFlagByValue(Flags::Carry, carry);
	SetFlagByValue(Flags::Zero, reg == 0);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 8;
}

void SM83::RRC(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	uint8_t carry = val & 1;
	val = ((val >> 1) | (carry << 7));

	m_Bus->WriteMemory(addr, val);

	SetFlagByValue(Flags::Carry, carry);
	SetFlagByValue(Flags::Zero, val == 0);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 16;
}

void SM83::SRA(uint8_t& reg) {
	uint8_t carry = reg & 1;
	uint8_t msb = reg & 0x80;

	reg = (reg >> 1) | msb;

	SetFlagByValue(Flags::Carry, carry);
	SetFlagByValue(Flags::Zero, reg == 0);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 8;
}

void SM83::SRA(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	uint8_t carry = val & 1;
	uint8_t msb = val & 0x80;

	val = (val >> 1) | msb;
	m_Bus->WriteMemory(addr, val);

	SetFlagByValue(Flags::Carry, carry);
	SetFlagByValue(Flags::Zero, val == 0);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Subtraction);

	m_LastOpCycles += 16;
}

void SM83::Reset() {
	m_Registers.pc = 0x0000;
	m_Registers.f = 0x00;
}

void SM83::Dump(FILE* stream) {
	std::println(stream, "{:04x}: af = {:04x} bc = {:04x} de = {:04x} hl = {:04x} [{:02x}] sp = {:04x} ime = {} if = {:08b} ie = {:08b} [{}]",
		m_Registers.pc, m_Registers.af, m_Registers.bc, m_Registers.de, m_Registers.hl, m_Bus->ReadMemory(m_Registers.hl), m_Registers.sp, m_IME,
		m_Bus->ReadMemory(0xff0f), m_Bus->ReadMemory(0xffff), dmg::debugger::DisassembleInstruction(m_Bus, m_Registers.pc));
}

void SM83::CBStep() {
	uint8_t opcode = Fetch8();
	switch (opcode) {
		// BIT
		case 0x40: BIT(0, m_Registers.b); break;
		case 0x41: BIT(0, m_Registers.c); break;
		case 0x42: BIT(0, m_Registers.d); break;
		case 0x43: BIT(0, m_Registers.e); break;
		case 0x44: BIT(0, m_Registers.h); break;
		case 0x45: BIT(0, m_Registers.l); break;
		case 0x46: BIT(0, m_Registers.hl); break;
		case 0x47: BIT(0, m_Registers.a); break;
		case 0x48: BIT(1, m_Registers.b); break;
		case 0x49: BIT(1, m_Registers.c); break;
		case 0x4a: BIT(1, m_Registers.d); break;
		case 0x4b: BIT(1, m_Registers.e); break;
		case 0x4c: BIT(1, m_Registers.h); break;
		case 0x4d: BIT(1, m_Registers.l); break;
		case 0x4e: BIT(1, m_Registers.hl); break;
		case 0x4f: BIT(1, m_Registers.a); break;
		case 0x50: BIT(2, m_Registers.b); break;
		case 0x51: BIT(2, m_Registers.c); break;
		case 0x52: BIT(2, m_Registers.d); break;
		case 0x53: BIT(2, m_Registers.e); break;
		case 0x54: BIT(2, m_Registers.h); break;
		case 0x55: BIT(2, m_Registers.l); break;
		case 0x56: BIT(2, m_Registers.hl); break;
		case 0x57: BIT(2, m_Registers.a); break;
		case 0x58: BIT(3, m_Registers.b); break;
		case 0x59: BIT(3, m_Registers.c); break;
		case 0x5a: BIT(3, m_Registers.d); break;
		case 0x5b: BIT(3, m_Registers.e); break;
		case 0x5c: BIT(3, m_Registers.h); break;
		case 0x5d: BIT(3, m_Registers.l); break;
		case 0x5e: BIT(3, m_Registers.hl); break;
		case 0x5f: BIT(3, m_Registers.a); break;
		case 0x60: BIT(4, m_Registers.b); break;
		case 0x61: BIT(4, m_Registers.c); break;
		case 0x62: BIT(4, m_Registers.d); break;
		case 0x63: BIT(4, m_Registers.e); break;
		case 0x64: BIT(4, m_Registers.h); break;
		case 0x65: BIT(4, m_Registers.l); break;
		case 0x66: BIT(4, m_Registers.hl); break;
		case 0x67: BIT(4, m_Registers.a); break;
		case 0x68: BIT(5, m_Registers.b); break;
		case 0x69: BIT(5, m_Registers.c); break;
		case 0x6a: BIT(5, m_Registers.d); break;
		case 0x6b: BIT(5, m_Registers.e); break;
		case 0x6c: BIT(5, m_Registers.h); break;
		case 0x6d: BIT(5, m_Registers.l); break;
		case 0x6e: BIT(5, m_Registers.hl); break;
		case 0x6f: BIT(5, m_Registers.a); break;
		case 0x70: BIT(6, m_Registers.b); break;
		case 0x71: BIT(6, m_Registers.c); break;
		case 0x72: BIT(6, m_Registers.d); break;
		case 0x73: BIT(6, m_Registers.e); break;
		case 0x74: BIT(6, m_Registers.h); break;
		case 0x75: BIT(6, m_Registers.l); break;
		case 0x76: BIT(6, m_Registers.hl); break;
		case 0x77: BIT(6, m_Registers.a); break;
		case 0x78: BIT(7, m_Registers.b); break;
		case 0x79: BIT(7, m_Registers.c); break;
		case 0x7a: BIT(7, m_Registers.d); break;
		case 0x7b: BIT(7, m_Registers.e); break;
		case 0x7c: BIT(7, m_Registers.h); break;
		case 0x7d: BIT(7, m_Registers.l); break;
		case 0x7e: BIT(7, m_Registers.hl); break;
		case 0x7f: BIT(7, m_Registers.a); break;

		// RL
		case 0x10: RL(m_Registers.b); break;
		case 0x11: RL(m_Registers.c); break;
		case 0x12: RL(m_Registers.d); break;
		case 0x13: RL(m_Registers.e); break;
		case 0x14: RL(m_Registers.h); break;
		case 0x15: RL(m_Registers.l); break;
		case 0x16: RL(m_Registers.hl); break;
		case 0x17: RL(m_Registers.a); break;

		// SWAP
		case 0x30: SWAP(m_Registers.b); break;
		case 0x31: SWAP(m_Registers.c); break;
		case 0x32: SWAP(m_Registers.d); break;
		case 0x33: SWAP(m_Registers.e); break;
		case 0x34: SWAP(m_Registers.h); break;
		case 0x35: SWAP(m_Registers.l); break;
		case 0x36: SWAP(m_Registers.hl); break;
		case 0x37: SWAP(m_Registers.a); break;

		// RES
		case 0x80: RES(0, m_Registers.b); break;
		case 0x81: RES(0, m_Registers.c); break;
		case 0x82: RES(0, m_Registers.d); break;
		case 0x83: RES(0, m_Registers.e); break;
		case 0x84: RES(0, m_Registers.h); break;
		case 0x85: RES(0, m_Registers.l); break;
		case 0x86: RES(0, m_Registers.hl); break;
		case 0x87: RES(0, m_Registers.a); break;
		case 0x88: RES(1, m_Registers.b); break;
		case 0x89: RES(1, m_Registers.c); break;
		case 0x8a: RES(1, m_Registers.d); break;
		case 0x8b: RES(1, m_Registers.e); break;
		case 0x8c: RES(1, m_Registers.h); break;
		case 0x8d: RES(1, m_Registers.l); break;
		case 0x8e: RES(1, m_Registers.hl); break;
		case 0x8f: RES(1, m_Registers.a); break;
		case 0x90: RES(2, m_Registers.b); break;
		case 0x91: RES(2, m_Registers.c); break;
		case 0x92: RES(2, m_Registers.d); break;
		case 0x93: RES(2, m_Registers.e); break;
		case 0x94: RES(2, m_Registers.h); break;
		case 0x95: RES(2, m_Registers.l); break;
		case 0x96: RES(2, m_Registers.hl); break;
		case 0x97: RES(2, m_Registers.a); break;
		case 0x98: RES(3, m_Registers.b); break;
		case 0x99: RES(3, m_Registers.c); break;
		case 0x9a: RES(3, m_Registers.d); break;
		case 0x9b: RES(3, m_Registers.e); break;
		case 0x9c: RES(3, m_Registers.h); break;
		case 0x9d: RES(3, m_Registers.l); break;
		case 0x9e: RES(3, m_Registers.hl); break;
		case 0x9f: RES(3, m_Registers.a); break;
		case 0xa0: RES(4, m_Registers.b); break;
		case 0xa1: RES(4, m_Registers.c); break;
		case 0xa2: RES(4, m_Registers.d); break;
		case 0xa3: RES(4, m_Registers.e); break;
		case 0xa4: RES(4, m_Registers.h); break;
		case 0xa5: RES(4, m_Registers.l); break;
		case 0xa6: RES(4, m_Registers.hl); break;
		case 0xa7: RES(4, m_Registers.a); break;
		case 0xa8: RES(5, m_Registers.b); break;
		case 0xa9: RES(5, m_Registers.c); break;
		case 0xaa: RES(5, m_Registers.d); break;
		case 0xab: RES(5, m_Registers.e); break;
		case 0xac: RES(5, m_Registers.h); break;
		case 0xad: RES(5, m_Registers.l); break;
		case 0xae: RES(5, m_Registers.hl); break;
		case 0xaf: RES(5, m_Registers.a); break;
		case 0xb0: RES(6, m_Registers.b); break;
		case 0xb1: RES(6, m_Registers.c); break;
		case 0xb2: RES(6, m_Registers.d); break;
		case 0xb3: RES(6, m_Registers.e); break;
		case 0xb4: RES(6, m_Registers.h); break;
		case 0xb5: RES(6, m_Registers.l); break;
		case 0xb6: RES(6, m_Registers.hl); break;
		case 0xb7: RES(6, m_Registers.a); break;
		case 0xb8: RES(7, m_Registers.b); break;
		case 0xb9: RES(7, m_Registers.c); break;
		case 0xba: RES(7, m_Registers.d); break;
		case 0xbb: RES(7, m_Registers.e); break;
		case 0xbc: RES(7, m_Registers.h); break;
		case 0xbd: RES(7, m_Registers.l); break;
		case 0xbe: RES(7, m_Registers.hl); break;
		case 0xbf: RES(7, m_Registers.a); break;

		// SET
		case 0xc0: SET(0, m_Registers.b); break;
		case 0xc1: SET(0, m_Registers.c); break;
		case 0xc2: SET(0, m_Registers.d); break;
		case 0xc3: SET(0, m_Registers.e); break;
		case 0xc4: SET(0, m_Registers.h); break;
		case 0xc5: SET(0, m_Registers.l); break;
		case 0xc6: SET(0, m_Registers.hl); break;
		case 0xc7: SET(0, m_Registers.a); break;
		case 0xc8: SET(1, m_Registers.b); break;
		case 0xc9: SET(1, m_Registers.c); break;
		case 0xca: SET(1, m_Registers.d); break;
		case 0xcb: SET(1, m_Registers.e); break;
		case 0xcc: SET(1, m_Registers.h); break;
		case 0xcd: SET(1, m_Registers.l); break;
		case 0xce: SET(1, m_Registers.hl); break;
		case 0xcf: SET(1, m_Registers.a); break;
		case 0xd0: SET(2, m_Registers.b); break;
		case 0xd1: SET(2, m_Registers.c); break;
		case 0xd2: SET(2, m_Registers.d); break;
		case 0xd3: SET(2, m_Registers.e); break;
		case 0xd4: SET(2, m_Registers.h); break;
		case 0xd5: SET(2, m_Registers.l); break;
		case 0xd6: SET(2, m_Registers.hl); break;
		case 0xd7: SET(2, m_Registers.a); break;
		case 0xd8: SET(3, m_Registers.b); break;
		case 0xd9: SET(3, m_Registers.c); break;
		case 0xda: SET(3, m_Registers.d); break;
		case 0xdb: SET(3, m_Registers.e); break;
		case 0xdc: SET(3, m_Registers.h); break;
		case 0xdd: SET(3, m_Registers.l); break;
		case 0xde: SET(3, m_Registers.hl); break;
		case 0xdf: SET(3, m_Registers.a); break;
		case 0xe0: SET(4, m_Registers.b); break;
		case 0xe1: SET(4, m_Registers.c); break;
		case 0xe2: SET(4, m_Registers.d); break;
		case 0xe3: SET(4, m_Registers.e); break;
		case 0xe4: SET(4, m_Registers.h); break;
		case 0xe5: SET(4, m_Registers.l); break;
		case 0xe6: SET(4, m_Registers.hl); break;
		case 0xe7: SET(4, m_Registers.a); break;
		case 0xe8: SET(5, m_Registers.b); break;
		case 0xe9: SET(5, m_Registers.c); break;
		case 0xea: SET(5, m_Registers.d); break;
		case 0xeb: SET(5, m_Registers.e); break;
		case 0xec: SET(5, m_Registers.h); break;
		case 0xed: SET(5, m_Registers.l); break;
		case 0xee: SET(5, m_Registers.hl); break;
		case 0xef: SET(5, m_Registers.a); break;
		case 0xf0: SET(6, m_Registers.b); break;
		case 0xf1: SET(6, m_Registers.c); break;
		case 0xf2: SET(6, m_Registers.d); break;
		case 0xf3: SET(6, m_Registers.e); break;
		case 0xf4: SET(6, m_Registers.h); break;
		case 0xf5: SET(6, m_Registers.l); break;
		case 0xf6: SET(6, m_Registers.hl); break;
		case 0xf7: SET(6, m_Registers.a); break;
		case 0xf8: SET(7, m_Registers.b); break;
		case 0xf9: SET(7, m_Registers.c); break;
		case 0xfa: SET(7, m_Registers.d); break;
		case 0xfb: SET(7, m_Registers.e); break;
		case 0xfc: SET(7, m_Registers.h); break;
		case 0xfd: SET(7, m_Registers.l); break;
		case 0xfe: SET(7, m_Registers.hl); break;
		case 0xff: SET(7, m_Registers.a); break;

		// SLA
		case 0x20: SLA(m_Registers.b); break;
		case 0x21: SLA(m_Registers.c); break;
		case 0x22: SLA(m_Registers.d); break;
		case 0x23: SLA(m_Registers.e); break;
		case 0x24: SLA(m_Registers.h); break;
		case 0x25: SLA(m_Registers.l); break;
		case 0x26: SLA(m_Registers.hl); break;
		case 0x27: SLA(m_Registers.a); break;

		// RLC
		case 0x00: RLC(m_Registers.b); break;
		case 0x01: RLC(m_Registers.c); break;
		case 0x02: RLC(m_Registers.d); break;
		case 0x03: RLC(m_Registers.e); break;
		case 0x04: RLC(m_Registers.h); break;
		case 0x05: RLC(m_Registers.l); break;
		case 0x06: RLC(m_Registers.hl); break;
		case 0x07: RLC(m_Registers.a); break;

		// SRL
		case 0x38: SRL(m_Registers.b); break;
		case 0x39: SRL(m_Registers.c); break;
		case 0x3a: SRL(m_Registers.d); break;
		case 0x3b: SRL(m_Registers.e); break;
		case 0x3c: SRL(m_Registers.h); break;
		case 0x3d: SRL(m_Registers.l); break;
		case 0x3e: SRL(m_Registers.hl); break;
		case 0x3f: SRL(m_Registers.a); break;

		// RR
		case 0x18: RR(m_Registers.b); break;
		case 0x19: RR(m_Registers.c); break;
		case 0x1a: RR(m_Registers.d); break;
		case 0x1b: RR(m_Registers.e); break;
		case 0x1c: RR(m_Registers.h); break;
		case 0x1d: RR(m_Registers.l); break;
		case 0x1e: RR(m_Registers.hl); break;
		case 0x1f: RR(m_Registers.a); break;

		// RRC
		case 0x08: RRC(m_Registers.b); break;
		case 0x09: RRC(m_Registers.c); break;
		case 0x0a: RRC(m_Registers.d); break;
		case 0x0b: RRC(m_Registers.e); break;
		case 0x0c: RRC(m_Registers.h); break;
		case 0x0d: RRC(m_Registers.l); break;
		case 0x0e: RRC(m_Registers.hl); break;
		case 0x0f: RRC(m_Registers.a); break;

		// SRA
		case 0x28: SRA(m_Registers.b); break;
		case 0x29: SRA(m_Registers.c); break;
		case 0x2a: SRA(m_Registers.d); break;
		case 0x2b: SRA(m_Registers.e); break;
		case 0x2c: SRA(m_Registers.h); break;
		case 0x2d: SRA(m_Registers.l); break;
		case 0x2e: SRA(m_Registers.hl); break;
		case 0x2f: SRA(m_Registers.a); break;

		default:
			m_Registers.pc -= 2;
			std::println(stderr, "cpu: unknown opcode cb {:02x} [{}]", opcode, dmg::debugger::DisassembleInstruction(m_Bus, m_Registers.pc));
			Dump(stderr);
			exit(1);
	}
}

uint8_t SM83::Step() {
	if (m_DumpInstruction) {
		Dump(stdout);
	}

	m_LastOpCycles = 0;

	bool enable_interrupts = m_EIqueued;
	m_EIqueued = false;

	// halt state
	if (m_State == State::Halt) {
		if ((m_Bus->GetIE() & m_Bus->GetIF() & 0x1f) != 0) {
			m_State = State::Normal;

			if (m_DontExecuteHandler) {
				m_DontExecuteHandler = false;
			} else {
				HandleInterrupts();
			}

			return m_LastOpCycles;
		}

		return 4;
	}

	// stop state
	if (m_State == State::Stop) {
		// if P10-P13 go low then wake up
		if ((m_Bus->ReadMemory(0xff00) & 0b1111) != 0b1111) {
			m_State = State::Normal;
		}

		return 4;
	}

	uint8_t opcode = Fetch8();
	if (m_DoubleRead) {
		m_Registers.pc--;
		m_DoubleRead = false;
	}

	switch (opcode) {
		// NOP
		case 0x00: m_LastOpCycles += 4; break;

		// LD
		case 0x01: LD(m_Registers.bc, Fetch16(), 12); break;
		case 0x02: LD(m_Registers.bc, m_Registers.a, 8); break;
		case 0x06: LD(m_Registers.b, Fetch8(), 8); break;
		case 0x08: LD_addr(Fetch16(), m_Registers.sp, 20); break;
		case 0x0a: LD(m_Registers.a, m_Registers.bc, 8); break;
		case 0x0e: LD(m_Registers.c, Fetch8(), 8); break;
		case 0x11: LD(m_Registers.de, Fetch16(), 12); break;
		case 0x12: LD(m_Registers.de, m_Registers.a, 8); break;
		case 0x16: LD(m_Registers.d, Fetch8(), 8); break;
		case 0x1a: LD(m_Registers.a, m_Registers.de, 8); break;
		case 0x1e: LD(m_Registers.e, Fetch8(), 8); break;
		case 0x21: LD(m_Registers.hl, Fetch16(), 12); break;
		case 0x22: LD(m_Registers.hl++, m_Registers.a, 8); break;
		case 0x26: LD(m_Registers.h, Fetch8(), 8); break;
		case 0x2a: LD(m_Registers.a, m_Registers.hl++, 8); break;
		case 0x2e: LD(m_Registers.l, Fetch8(), 8); break;
		case 0x31: LD(m_Registers.sp, Fetch16(), 12); break;
		case 0x32: LD(m_Registers.hl--, m_Registers.a, 8); break;
		case 0x36: LD(m_Registers.hl, Fetch8(), 12); break;
		case 0x3a: LD(m_Registers.a, m_Registers.hl--, 8); break;
		case 0x3e: LD(m_Registers.a, Fetch8(), 8); break;
		case 0x40: LD(m_Registers.b, m_Registers.b, 4); break;
		case 0x41: LD(m_Registers.b, m_Registers.c, 4); break;
		case 0x42: LD(m_Registers.b, m_Registers.d, 4); break;
		case 0x43: LD(m_Registers.b, m_Registers.e, 4); break;
		case 0x44: LD(m_Registers.b, m_Registers.h, 4); break;
		case 0x45: LD(m_Registers.b, m_Registers.l, 4); break;
		case 0x46: LD(m_Registers.b, m_Registers.hl, 8); break;
		case 0x47: LD(m_Registers.b, m_Registers.a, 4); break;
		case 0x48: LD(m_Registers.c, m_Registers.b, 4); break;
		case 0x49: LD(m_Registers.c, m_Registers.c, 4); break;
		case 0x4a: LD(m_Registers.c, m_Registers.d, 4); break;
		case 0x4b: LD(m_Registers.c, m_Registers.e, 4); break;
		case 0x4c: LD(m_Registers.c, m_Registers.h, 4); break;
		case 0x4d: LD(m_Registers.c, m_Registers.l, 4); break;
		case 0x4e: LD(m_Registers.c, m_Registers.hl, 8); break;
		case 0x4f: LD(m_Registers.c, m_Registers.a, 4); break;
		case 0x50: LD(m_Registers.d, m_Registers.b, 4); break;
		case 0x51: LD(m_Registers.d, m_Registers.c, 4); break;
		case 0x52: LD(m_Registers.d, m_Registers.d, 4); break;
		case 0x53: LD(m_Registers.d, m_Registers.e, 4); break;
		case 0x54: LD(m_Registers.d, m_Registers.h, 4); break;
		case 0x55: LD(m_Registers.d, m_Registers.l, 4); break;
		case 0x56: LD(m_Registers.d, m_Registers.hl, 8); break;
		case 0x57: LD(m_Registers.d, m_Registers.a, 4); break;
		case 0x58: LD(m_Registers.e, m_Registers.b, 4); break;
		case 0x59: LD(m_Registers.e, m_Registers.c, 4); break;
		case 0x5a: LD(m_Registers.e, m_Registers.d, 4); break;
		case 0x5b: LD(m_Registers.e, m_Registers.e, 4); break;
		case 0x5c: LD(m_Registers.e, m_Registers.h, 4); break;
		case 0x5d: LD(m_Registers.e, m_Registers.l, 4); break;
		case 0x5e: LD(m_Registers.e, m_Registers.hl, 8); break;
		case 0x5f: LD(m_Registers.e, m_Registers.a, 4); break;
		case 0x60: LD(m_Registers.h, m_Registers.b, 4); break;
		case 0x61: LD(m_Registers.h, m_Registers.c, 4); break;
		case 0x62: LD(m_Registers.h, m_Registers.d, 4); break;
		case 0x63: LD(m_Registers.h, m_Registers.e, 4); break;
		case 0x64: LD(m_Registers.h, m_Registers.h, 4); break;
		case 0x65: LD(m_Registers.h, m_Registers.l, 4); break;
		case 0x66: LD(m_Registers.h, m_Registers.hl, 8); break;
		case 0x67: LD(m_Registers.h, m_Registers.a, 4); break;
		case 0x68: LD(m_Registers.l, m_Registers.b, 4); break;
		case 0x69: LD(m_Registers.l, m_Registers.c, 4); break;
		case 0x6a: LD(m_Registers.l, m_Registers.d, 4); break;
		case 0x6b: LD(m_Registers.l, m_Registers.e, 4); break;
		case 0x6c: LD(m_Registers.l, m_Registers.h, 4); break;
		case 0x6d: LD(m_Registers.l, m_Registers.l, 4); break;
		case 0x6e: LD(m_Registers.l, m_Registers.hl, 8); break;
		case 0x6f: LD(m_Registers.l, m_Registers.a, 4); break;
		case 0x70: LD(m_Registers.hl, m_Registers.b, 8); break;
		case 0x71: LD(m_Registers.hl, m_Registers.c, 8); break;
		case 0x72: LD(m_Registers.hl, m_Registers.d, 8); break;
		case 0x73: LD(m_Registers.hl, m_Registers.e, 8); break;
		case 0x74: LD(m_Registers.hl, m_Registers.h, 8); break;
		case 0x75: LD(m_Registers.hl, m_Registers.l, 8); break;
		case 0x77: LD(m_Registers.hl, m_Registers.a, 8); break;
		case 0x78: LD(m_Registers.a, m_Registers.b, 4); break;
		case 0x79: LD(m_Registers.a, m_Registers.c, 4); break;
		case 0x7a: LD(m_Registers.a, m_Registers.d, 4); break;
		case 0x7b: LD(m_Registers.a, m_Registers.e, 4); break;
		case 0x7c: LD(m_Registers.a, m_Registers.h, 4); break;
		case 0x7d: LD(m_Registers.a, m_Registers.l, 4); break;
		case 0x7e: LD(m_Registers.a, m_Registers.hl, 8); break;
		case 0x7f: LD(m_Registers.a, m_Registers.a, 4); break;
		case 0xea: LD(Fetch16(), m_Registers.a, 16); break;
		case 0xf8: LD(m_Registers.hl, m_Registers.sp, Fetch8(), 12); break;
		case 0xf9: LD(m_Registers.sp, m_Registers.hl, 8); break;
		case 0xfa: LD(m_Registers.a, m_Bus->ReadMemory(Fetch16()), 16); break;

		// INC
		case 0x03: INC(m_Registers.bc); break;
		case 0x04: INC(m_Registers.b); break;
		case 0x0c: INC(m_Registers.c); break;
		case 0x13: INC(m_Registers.de); break;
		case 0x14: INC(m_Registers.d); break;
		case 0x1c: INC(m_Registers.e); break;
		case 0x23: INC(m_Registers.hl); break;
		case 0x24: INC(m_Registers.h); break;
		case 0x2c: INC(m_Registers.l); break;
		case 0x33: INC(m_Registers.sp); break;
		case 0x34: INC_addr(m_Registers.hl); break;
		case 0x3c: INC(m_Registers.a); break;

		// DEC
		case 0x05: DEC(m_Registers.b); break;
		case 0x0b: DEC(m_Registers.bc); break;
		case 0x0d: DEC(m_Registers.c); break;
		case 0x15: DEC(m_Registers.d); break;
		case 0x1b: DEC(m_Registers.de); break;
		case 0x1d: DEC(m_Registers.e); break;
		case 0x25: DEC(m_Registers.h); break;
		case 0x2b: DEC(m_Registers.hl); break;
		case 0x2d: DEC(m_Registers.l); break;
		case 0x35: DEC_addr(m_Registers.hl); break;
		case 0x3b: DEC(m_Registers.sp); break;
		case 0x3d: DEC(m_Registers.a); break;

		// XOR
		case 0xa8: XOR(m_Registers.a, m_Registers.b, 4); break;
		case 0xa9: XOR(m_Registers.a, m_Registers.c, 4); break;
		case 0xaa: XOR(m_Registers.a, m_Registers.d, 4); break;
		case 0xab: XOR(m_Registers.a, m_Registers.e, 4); break;
		case 0xac: XOR(m_Registers.a, m_Registers.h, 4); break;
		case 0xad: XOR(m_Registers.a, m_Registers.l, 4); break;
		case 0xae: XOR(m_Registers.a, m_Registers.hl, 8); break;
		case 0xaf: XOR(m_Registers.a, m_Registers.a, 4); break;
		case 0xee: XOR(m_Registers.a, Fetch8(), 8); break;

		// JR
		case 0x18: JR(Fetch8()); break;
		case 0x20: JR(Flags::Zero, true, Fetch8()); break;
		case 0x28: JR(Flags::Zero, false, Fetch8()); break;
		case 0x30: JR(Flags::Carry, true, Fetch8()); break;
		case 0x38: JR(Flags::Carry, false, Fetch8()); break;

		// LDH
		case 0xe0: LD(0xff00 + Fetch8(), m_Registers.a, 12); break;
		case 0xe2: LD(0xff00 + m_Registers.c, m_Registers.a, 8); break;
		case 0xf0: LD(m_Registers.a, static_cast<uint16_t>(0xff00 + Fetch8()), 12); break;
		case 0xf2: LD(m_Registers.a, static_cast<uint16_t>(0xff00 + m_Registers.c), 8); break;

		// CALL
		case 0xc4: CALL(Flags::Zero, true, Fetch16()); break;
		case 0xcc: CALL(Flags::Zero, false, Fetch16()); break;
		case 0xcd: CALL(Fetch16()); break;
		case 0xd4: CALL(Flags::Carry, true, Fetch16()); break;
		case 0xdc: CALL(Flags::Carry, false, Fetch16()); break;

		// PUSH
		case 0xc5: PUSH(m_Registers.bc); break;
		case 0xd5: PUSH(m_Registers.de); break;
		case 0xe5: PUSH(m_Registers.hl); break;
		case 0xf5: PUSH(m_Registers.af); break;

		// RLA
		case 0x17: RLA(); break;

		// POP
		case 0xc1: POP(m_Registers.bc); break;
		case 0xd1: POP(m_Registers.de); break;
		case 0xe1: POP(m_Registers.hl); break;
		case 0xf1: POP_af(); break;

		// RET
		case 0xc0: RET(Flags::Zero, true); break;
		case 0xc8: RET(Flags::Zero, false); break;
		case 0xc9: RET(); break;
		case 0xd0: RET(Flags::Carry, true); break;
		case 0xd8: RET(Flags::Carry, false); break;

		// CP
		case 0xb8: CP(m_Registers.b, 4); break;
		case 0xb9: CP(m_Registers.c, 4); break;
		case 0xba: CP(m_Registers.d, 4); break;
		case 0xbb: CP(m_Registers.e, 4); break;
		case 0xbc: CP(m_Registers.h, 4); break;
		case 0xbd: CP(m_Registers.l, 4); break;
		case 0xbe: CP(m_Registers.hl, 8); break;
		case 0xbf: CP(m_Registers.a, 4); break;
		case 0xfe: CP(Fetch8(), 8); break;

		// JP
		case 0xc2: JP(Flags::Zero, true, Fetch16()); break;
		case 0xc3: JP(Fetch16(), 16); break;
		case 0xca: JP(Flags::Zero, false, Fetch16()); break;
		case 0xd2: JP(Flags::Carry, true, Fetch16()); break;
		case 0xda: JP(Flags::Carry, false, Fetch16()); break;
		case 0xe9: JP(m_Registers.hl, 4); break;

		// DI
		case 0xf3: DI(); break;

		// EI
		case 0xfb: EI(); break;

		// SUB
		case 0x90: SUB(m_Registers.b, 4); break;
		case 0x91: SUB(m_Registers.c, 4); break;
		case 0x92: SUB(m_Registers.d, 4); break;
		case 0x93: SUB(m_Registers.e, 4); break;
		case 0x94: SUB(m_Registers.h, 4); break;
		case 0x95: SUB(m_Registers.l, 4); break;
		case 0x96: SUB(m_Registers.hl); break;
		case 0x97: SUB(m_Registers.a, 4); break;
		case 0xd6: SUB(Fetch8(), 8); break;

		// ADD
		case 0x09: ADDhl(m_Registers.bc); break;
		case 0x19: ADDhl(m_Registers.de); break;
		case 0x29: ADDhl(m_Registers.hl); break;
		case 0x39: ADDhl(m_Registers.sp); break;
		case 0x80: ADD(m_Registers.b, 4); break;
		case 0x81: ADD(m_Registers.c, 4); break;
		case 0x82: ADD(m_Registers.d, 4); break;
		case 0x83: ADD(m_Registers.e, 4); break;
		case 0x84: ADD(m_Registers.h, 4); break;
		case 0x85: ADD(m_Registers.l, 4); break;
		case 0x86: ADD(m_Registers.hl); break;
		case 0x87: ADD(m_Registers.a, 4); break;
		case 0xc6: ADD(Fetch8(), 8); break;
		case 0xe8: ADD(static_cast<int8_t>(Fetch8())); break;

		// OR
		case 0xb0: OR(m_Registers.b, 4); break;
		case 0xb1: OR(m_Registers.c, 4); break;
		case 0xb2: OR(m_Registers.d, 4); break;
		case 0xb3: OR(m_Registers.e, 4); break;
		case 0xb4: OR(m_Registers.h, 4); break;
		case 0xb5: OR(m_Registers.l, 4); break;
		case 0xb6: OR(m_Registers.hl); break;
		case 0xb7: OR(m_Registers.a, 4); break;
		case 0xf6: OR(Fetch8(), 8); break;

		// CPL
		case 0x2f: CPL(); break;

		// AND
		case 0xa0: AND(m_Registers.b, 4); break;
		case 0xa1: AND(m_Registers.c, 4); break;
		case 0xa2: AND(m_Registers.d, 4); break;
		case 0xa3: AND(m_Registers.e, 4); break;
		case 0xa4: AND(m_Registers.h, 4); break;
		case 0xa5: AND(m_Registers.l, 4); break;
		case 0xa6: AND(m_Registers.hl); break;
		case 0xa7: AND(m_Registers.a, 4); break;
		case 0xe6: AND(Fetch8(), 8); break;

		// RST
		case 0xc7: RST(0x00); break;
		case 0xcf: RST(0x08); break;
		case 0xd7: RST(0x10); break;
		case 0xdf: RST(0x18); break;
		case 0xe7: RST(0x20); break;
		case 0xef: RST(0x28); break;
		case 0xf7: RST(0x30); break;
		case 0xff: RST(0x38); break;

		// ADC
		case 0x88: ADC(m_Registers.b, 4); break;
		case 0x89: ADC(m_Registers.c, 4); break;
		case 0x8a: ADC(m_Registers.d, 4); break;
		case 0x8b: ADC(m_Registers.e, 4); break;
		case 0x8c: ADC(m_Registers.h, 4); break;
		case 0x8d: ADC(m_Registers.l, 4); break;
		case 0x8e: ADC(m_Bus->ReadMemory(m_Registers.hl), 8); break;
		case 0x8f: ADC(m_Registers.a, 4); break;
		case 0xce: ADC(Fetch8(), 8); break;

		// HALT
		case 0x76: HALT(); break;

		// RETI
		case 0xd9: RETI(); break;

		// RLCA
		case 0x07: RLCA(); break;

		// DAA
		case 0x27: DAA(); break;

		// RRA
		case 0x1f: RRA(); break;

		// CCF
		case 0x3f: CCF(); break;

		// STOP
		case 0x10: STOP(); Fetch8(); break;

		// SCF
		case 0x37: SCF(); break;

		// SBC
		case 0x98: SBC(m_Registers.b, 4); break;
		case 0x99: SBC(m_Registers.c, 4); break;
		case 0x9a: SBC(m_Registers.d, 4); break;
		case 0x9b: SBC(m_Registers.e, 4); break;
		case 0x9c: SBC(m_Registers.h, 4); break;
		case 0x9d: SBC(m_Registers.l, 4); break;
		case 0x9e: SBC(m_Bus->ReadMemory(m_Registers.hl), 8); break;
		case 0x9f: SBC(m_Registers.a, 4); break;
		case 0xde: SBC(Fetch8(), 8); break;

		// RRCA
		case 0x0f: RRCA(); break;

		// CB prefix
		case 0xcb: CBStep(); break;
		
		default:
			std::println(stderr, "cpu: unknown opcode {:02x} [{}]", opcode, dmg::debugger::DisassembleInstruction(m_Bus, --m_Registers.pc));
			Dump(stderr);
			exit(1);
	}

	// handle any interrupts
	HandleInterrupts();

	// set the IME if it is queued
	if (enable_interrupts) {
		m_IME = true;
	}

	return m_LastOpCycles;
}

void SM83::HandleInterrupts() {
	// interrupts sorted by their priority
	static const uint8_t int_priorities[] = {
		dmg::bus::InterruptFlag::VBlank,
		dmg::bus::InterruptFlag::LCD,
		dmg::bus::InterruptFlag::Timer,
		dmg::bus::InterruptFlag::Serial,
		dmg::bus::InterruptFlag::Joypad,
	};

	// interrupt handler addresses
	static const uint16_t int_handlers[] = {
		0x0040,
		0x0048,
		0x0050,
		0x0058,
		0x0060,
	};

    uint8_t pending = m_Bus->GetIF() & m_Bus->GetIE() & 0x1f;
	if (!m_IME) return;
    if (!pending) return;

	for (size_t i = 0; i < 5; i++) {
        uint8_t mask = int_priorities[i];
        if (pending & mask) {
			//std::println("cpu: servicing interrupt {}", i+1);
            m_Bus->SetIF(m_Bus->GetIF() & ~mask);
            m_IME = false;
            
            StackPush16(m_Registers.pc);
            m_Registers.pc = int_handlers[i];
            m_LastOpCycles += 20;
            break;
        }
    }
}