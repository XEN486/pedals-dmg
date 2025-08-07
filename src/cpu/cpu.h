#ifndef SM83_H
#define SM83_H

#if defined(_MSC_VER)
#pragma warning(disable: 4201)
#endif

#include "../peripherals/bus.h"
#include "debugger.h"

#include <stdint.h>
#include <memory>

// this uses a nameless struct which is not standard and only works on little-endian host platforms
#define RegisterPair(h, l) \
	union { \
		uint16_t h##l; \
		struct { \
			uint8_t l; \
			uint8_t h; \
		}; \
	}

namespace pedals::cpu {
	enum Flags : uint8_t {
		Zero		= 0b10000000,
		Subtraction	= 0b01000000,
		HalfCarry	= 0b00100000,
		Carry		= 0b00010000,
	};

	struct Registers {
		RegisterPair(a, f);
		RegisterPair(b, c);
		RegisterPair(d, e);
		RegisterPair(h, l);

		uint16_t sp;
		uint16_t pc;
	};

	enum class State {
		Normal,
		Halt,
		Stop
	};

	class SM83 {
	public:
		SM83(std::shared_ptr<pedals::bus::Bus> bus) : m_Bus(bus) {}
		void Reset();
		void Dump(FILE* stream);

		// returns the T-cycles the step took
		uint8_t Step();

		bool InInterrupt() const {
			return m_HandlingInterrupt;
		}

	public:
		Registers& GetRegistersRef() {
			return m_Registers;
		}
		
		bool& GetIMERef() {
			return m_IME;
		}

		bool& GetRETIRef() {
			return m_RETI;
		}

	private:
		inline bool GetFlag(Flags flag) {
			return (m_Registers.f & flag) != 0;
		}

		inline void SetFlag(Flags flag) {
			m_Registers.f |= flag;
		}
		
		inline void ClearFlag(Flags flag) {
			m_Registers.f &= ~flag;
		}

		void SetFlagByValue(Flags flag, bool condition) {
			if (condition) SetFlag(flag);
			else ClearFlag(flag);
		}

	private:
		void LD(uint16_t& dst, uint16_t src, uint8_t cycles);
		void LD(uint8_t& dst, uint8_t src, uint8_t cycles);
		void LD(uint16_t addr, uint8_t src, uint8_t cycles);
		void LD(uint8_t& dst, uint16_t addr, uint8_t cycles);
		void LD(uint16_t& dst, uint16_t src, int8_t n, uint8_t cycles);
		void LD_addr(uint16_t addr, uint16_t src, uint8_t cycles);

		void INC(uint8_t& reg);
		void INC(uint16_t& reg);
		void INC_addr(uint16_t addr);

		void DEC(uint8_t& reg);
		void DEC(uint16_t& reg);
		void DEC_addr(uint16_t addr);

		void XOR(uint8_t& reg, uint8_t src, uint8_t cycles);
		void XOR(uint8_t& reg, uint16_t addr, uint8_t cycles);

		void JR(int8_t rel);
		void JR(Flags flag, bool inverse, int8_t rel);

		void LDH(uint8_t dst, uint8_t src, uint8_t cycles);
		
		void CALL(uint16_t addr);
		void CALL(Flags flag, bool inverse, uint16_t addr);

		void PUSH(uint16_t reg);

		void RLA();

		void POP(uint16_t& reg);
		void POP_af();

		void RET();
		void RET(Flags flag, bool inverse);

		void CP(uint8_t value, uint8_t cycles);
		void CP(uint16_t addr, uint8_t cycles);

		void JP(uint16_t addr, uint8_t cycles);
		void JP(Flags flag, bool inverse, uint16_t addr);

		void DI();
	
		void EI();

		void RETI();

		void SUB(uint8_t value, uint8_t cycles);
		void SUB(uint16_t addr);

		void ADD(uint8_t value, uint8_t cycles);
		void ADD(uint16_t addr);
		void ADDhl(uint16_t reg);
		void ADD(int8_t value);

		void OR(uint8_t src, uint8_t cycles);
		void OR(uint16_t addr);

		void CPL();

		void AND(uint8_t src, uint8_t cycles);
		void AND(uint16_t addr);

		void RST(uint8_t vec);

		void ADC(uint8_t value, uint8_t cycles);

		void HALT();

		void RLCA();

		void DAA();

		void RRA();

		void CCF();

		void STOP();

		void SCF();

		void SBC(uint8_t value, uint8_t cycles);

		void RRCA();

	private:
		void BIT(uint8_t u3, uint8_t reg);
		void BIT(uint8_t u3, uint16_t addr);

		void RL(uint8_t& reg);
		void RL(uint16_t addr);

		void SWAP(uint8_t& reg);
		void SWAP(uint16_t addr);

		void RES(uint8_t u3, uint8_t& reg);
		void RES(uint8_t u3, uint16_t addr);

		void SET(uint8_t u3, uint8_t& reg);
		void SET(uint8_t u3, uint16_t addr);

		void SLA(uint8_t& reg);
		void SLA(uint16_t addr);

		void RLC(uint8_t& reg);
		void RLC(uint16_t addr);

		void SRL(uint8_t& reg);
		void SRL(uint16_t addr);

		void RR(uint8_t& reg);
		void RR(uint16_t addr);

		void RRC(uint8_t& reg);
		void RRC(uint16_t addr);
		
		void SRA(uint8_t& reg);
		void SRA(uint16_t addr);

	public:
		inline void StackPush8(uint8_t data) {
			m_Bus->WriteMemory(--m_Registers.sp, data);
		}

		inline void StackPush16(uint16_t data) {
			StackPush8(data >> 8);
			StackPush8(data & 0xff);
		}

		inline uint8_t StackPop8() {
			return m_Bus->ReadMemory(m_Registers.sp++);
		}

		inline uint16_t StackPop16() {
			uint8_t lo = StackPop8();
			uint8_t hi = StackPop8();
			return (hi << 8) | lo;
		}

	private:
		void HandleInterrupts();
		void CBStep();

		inline uint8_t Fetch8() {
			return m_Bus->ReadMemory(m_Registers.pc++);
		}

		inline uint16_t Fetch16() {
			m_Registers.pc += 2;
			return m_Bus->ReadMemory16(m_Registers.pc - 2);
		}

	private:
		Registers m_Registers;
		std::shared_ptr<pedals::bus::Bus> m_Bus;
		uint8_t m_LastOpCycles = 0;
		bool m_IME = false;
		bool m_EIqueued = false;

		State m_State = State::Normal;
		bool m_DoubleRead = false;
		bool m_DontExecuteHandler = false;
		
		bool m_HandlingInterrupt = false;
		bool m_RETI = false;
	};
}

#endif