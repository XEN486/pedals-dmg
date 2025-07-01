#include "sm83.h"
#include <print>

using namespace gb::cpu;

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

void SM83::LD(uint16_t& dst, uint16_t src, uint8_t n, uint8_t cycles) {
	uint16_t result = src + static_cast<int8_t>(n);
	
	ClearFlag(Flags::Zero);
	ClearFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((src ^ n ^ result) & 0x10) == 0x10);
	SetFlagByValue(Flags::Carry, ((src ^ n ^ result) & 0x100) == 0x100);
	
	dst = result;
	m_LastOpCycles += cycles;
}

void SM83::INC(uint8_t& reg) {
	SetFlagByValue(Flags::Zero, (reg + 1) == 0);
	ClearFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, (reg & 0xF) == 0xF);
	
	reg++;
	m_LastOpCycles += 4;
}

void SM83::INC(uint16_t& reg) {
	reg++;
	m_LastOpCycles += 8;
}

void SM83::INC_addr(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	
	SetFlagByValue(Flags::Zero, (val + 1) == 0);
	ClearFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, (val & 0xF) == 0xF);
	
	m_Bus->WriteMemory(addr, val + 1);
	m_LastOpCycles += 12;
}

void SM83::DEC(uint8_t& reg) {
	SetFlagByValue(Flags::Zero, (reg - 1) == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, (reg & 0xF) == 0);
	
	reg--;
	m_LastOpCycles += 4;
}

void SM83::DEC(uint16_t& reg) {
	reg--;
	m_LastOpCycles += 8;
}

void SM83::DEC_addr(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	
	SetFlagByValue(Flags::Zero, (val - 1) == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, (val & 0xF) == 0);
	
	m_Bus->WriteMemory(addr, val - 1);
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
	if ((m_Registers.f & flag) != (inverse ? flag : 0)) {
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

	if ((m_Registers.f & flag) != (inverse ? flag : 0)) {
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
	m_Registers.a = (m_Registers.a << 1) | ((m_Registers.f & Flags::Carry) ? 1 : 0);
	
	SetFlagByValue(Flags::Carry, new_carry);
	ClearFlag(Flags::Zero);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);

	m_LastOpCycles += 8;
}


void SM83::POP(uint16_t& reg) {
	reg = StackPop16();
	m_LastOpCycles += 12;
}

void SM83::RET() {
	m_Registers.pc = StackPop16();
	m_LastOpCycles += 16;
}

void SM83::CP(uint8_t value, uint8_t cycles) {
	uint8_t result = value - m_Registers.a;

	SetFlagByValue(Flags::Zero, result == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((m_Registers.a & 0xf) - (value & 0xf)) < 0);
	SetFlagByValue(Flags::Carry, m_Registers.a < value);

	m_LastOpCycles += cycles;
}

void SM83::CP(uint16_t addr, uint8_t cycles) {
	uint8_t value = m_Bus->ReadMemory(addr);
	uint8_t result = value - m_Registers.a;
	
	SetFlagByValue(Flags::Zero, result == 0);
	SetFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((m_Registers.a & 0xf) - (value & 0xf)) < 0);
	SetFlagByValue(Flags::Carry, m_Registers.a < value);

	m_LastOpCycles += cycles;
}

void SM83::JP(uint16_t addr, uint8_t cycles) {
	m_Registers.pc = addr;
	m_LastOpCycles += cycles;
}

void SM83::JP(Flags flag, bool inverse, uint16_t addr) {
	m_LastOpCycles += 12;

	if ((m_Registers.f & flag) != (inverse ? flag : 0)) {
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
	SetFlagByValue(Flags::HalfCarry, (r & 0xfff) + (reg & 0xfff) > 0xfff);
	SetFlagByValue(Flags::Carry, (result & 0x10000) != 0);
	m_Registers.hl = static_cast<uint16_t>(result);

	m_LastOpCycles += 8;
}

void SM83::ADD(int8_t value) {
	int result = value + m_Registers.sp;

	ClearFlag(Flags::Zero);
	ClearFlag(Flags::Subtraction);
	SetFlagByValue(Flags::HalfCarry, ((m_Registers.sp & 0xF) + (value & 0xF)) > 0xF);
	SetFlagByValue(Flags::Carry, ((m_Registers.sp & 0xFF) + (value & 0xFF)) > 0xFF);

	m_Registers.sp = static_cast<uint16_t>(result);
	m_LastOpCycles += 16;
}

void SM83::OR(uint8_t src, uint8_t cycles) {
	m_Registers.a |= src;

	SetFlagByValue(Flags::Zero, m_Registers.a);
	ClearFlag(Flags::Subtraction);
	ClearFlag(Flags::HalfCarry);
	ClearFlag(Flags::Carry);

	m_LastOpCycles += cycles;
}

void SM83::OR(uint16_t addr) {
	m_Registers.a |= m_Bus->ReadMemory(addr);
	
	SetFlagByValue(Flags::Zero, m_Registers.a);
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
	SetFlagByValue(Flags::Zero, m_Registers.a);
	SetFlag(Flags::HalfCarry);

	m_LastOpCycles += cycles;
}

void SM83::AND(uint16_t addr) {
	m_Registers.a &= m_Bus->ReadMemory(addr);
	SetFlagByValue(Flags::Zero, m_Registers.a);
	SetFlag(Flags::HalfCarry);

	m_LastOpCycles += 8;
}

void SM83::RST(uint8_t vec) {
	StackPush16(m_Registers.pc);

	m_Registers.pc = vec;
	m_LastOpCycles += 4;
}

void SM83::BIT(uint8_t bit, uint8_t reg) {
	SetFlagByValue(Flags::Zero, (reg & (1 << bit)) == 0);
	ClearFlag(Flags::Subtraction);
	SetFlag(Flags::HalfCarry);

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

	m_LastOpCycles += 8;
}

void SM83::SWAP(uint16_t addr) {
	uint8_t val = m_Bus->ReadMemory(addr);
	val = (val << 4) | (val >> 4);

	m_Bus->WriteMemory(addr, val);
	SetFlagByValue(Flags::Zero, val == 0);

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

void SM83::Reset() {
	m_Registers.pc = 0x0000;
}

void SM83::Dump(FILE* stream) {
	std::println(stream, "af = {:04x} bc = {:04x} de = {:04x} hl = {:04x} sp = {:04x} pc = {:04x}",
		m_Registers.af, m_Registers.bc, m_Registers.de, m_Registers.hl, m_Registers.sp, m_Registers.pc);
}

void SM83::SetDMGBootROMState() {
	// set registers to their states after the boot rom finishes
	// these values are for the DMG boot rom
	m_Registers.af = 0x0100;
	m_Registers.bc = 0x0013;
	m_Registers.de = 0x00d8;
	m_Registers.hl = 0x014d;
	m_Registers.pc = 0x0100;
	m_Registers.sp = 0xfffe;

	// the F register depends on the header checksum
	SetFlag(Flags::Zero);
	if (m_Bus->GetHeaderChecksum() != 0) {
		SetFlag(Flags::Subtraction);
		SetFlag(Flags::HalfCarry);
	}

	// disable reading boot rom
	m_Bus->SetBootROMVisibility(false);
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

		default:
			std::println(stderr, "unknown cb-prefix opcode {:x}", opcode);
			Dump(stderr);
			exit(1);
	}
}

uint8_t SM83::Step() {
	m_LastOpCycles = 0;
step:
	uint8_t opcode = Fetch8();
	switch (opcode) {
		// NOP
		case 0x00: break;

		// LD
		case 0x01: LD(m_Registers.bc, Fetch16(), 12); break;
		case 0x02: LD(m_Registers.bc, m_Registers.a, 8); break;
		case 0x06: LD(m_Registers.b, Fetch8(), 8); break;
		case 0x08: LD(Fetch16(), m_Registers.sp, 20); break;
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
		case 0xf8: LD(m_Registers.hl, m_Registers.sp++, Fetch8(), 12); break;
		case 0xf9: LD(m_Registers.sp, m_Registers.hl, 8); break;
		case 0xfa: LD(m_Registers.a, Fetch16(), 16); break;

		// INC
		case 0x03: INC(m_Registers.bc); break;
		case 0x04: INC(m_Registers.bc); break;
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
		case 0xf1: POP(m_Registers.af); break;

		// RET
		case 0xc9: RET(); break;

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
		case 0xfb: EI(); goto step;

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

		// CB prefix
		case 0xcb: CBStep(); break;
		
		default:
			std::println(stderr, "cpu: unknown opcode {:x}", opcode);
			Dump(stderr);
			exit(1);
	}

	// set the IME if it is queued
	if (m_EIqueued) {
		m_IME = true;
	}

	// handle any interrupts
	HandleInterrupts();

	return m_LastOpCycles;
}

void SM83::HandleInterrupts() {
	// interrupts sorted by their priority
	static const uint8_t int_priorities[] = {
		gb::bus::InterruptFlag::VBlank,
		gb::bus::InterruptFlag::LCD,
		gb::bus::InterruptFlag::Timer,
		gb::bus::InterruptFlag::Serial,
		gb::bus::InterruptFlag::Joypad,
	};

	// interrupt handler addresses
	static const uint16_t int_handlers[] = {
		0x0040,
		0x0048,
		0x0050,
		0x0058,
		0x0060,
	};

	size_t i = 0;
	for (const uint8_t interrupt : int_priorities) {
		if (m_IME && ((m_Bus->GetIE() & interrupt) != 0) && ((m_Bus->GetIF() & interrupt) != 0)) {
			std::println("cpu: servicing interrupt {}", i+1);

			// acknowledge interrupt
			m_Bus->SetIF(m_Bus->GetIF() & ~interrupt);
			m_IME = false;

			// call the interrupt handler
			StackPush16(m_Registers.pc);
			m_Registers.pc = int_handlers[i];

			// calling the interrupt handler takes up five m-cycles
			m_LastOpCycles += 20;

			break;
		}

		i++;
	}
}