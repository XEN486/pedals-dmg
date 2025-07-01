#ifndef BUS_H
#define BUS_H

#include "ppu.h"

#include <stdint.h>
#include <memory>
#include <vector>
#include <print>

namespace gb::bus {
	enum InterruptFlag : uint8_t {
		Joypad	= 0b00010000,
		Serial	= 0b00001000,
		Timer	= 0b00000100,
		LCD		= 0b00000010,
		VBlank	= 0b00000001,
	};

	class Bus {
	public:
		Bus(std::shared_ptr<gb::ppu::PPU> ppu) :
			m_PPU(ppu),
			m_BootROM(256, 0),
			m_CurrentExternalRAMBank(0x2000, 0),
			m_WorkRAM(0x2000, 0),
			m_HighRAM(0x7f, 0) {}

		uint8_t ReadMemory(uint16_t address);
		void WriteMemory(uint16_t address, uint8_t value);

		void LoadBootROM(const std::vector<uint8_t>& rom) {
			m_BootROM = rom;
		}

		void LoadCartridgeBank0(uint8_t* bank) {
			m_CartridgeBank0 = bank;
		}

		void LoadCartridgeBank(uint8_t* bank) {
			m_CurrentCartridgeBank = bank;
		}

		void LoadExternalRAMBank(const std::vector<uint8_t>& bank) {
			m_CurrentExternalRAMBank = bank;
		}

		void SetBootROMVisibility(bool enable) {
			m_DisableBootROM = !enable;
		}
		
		uint16_t ReadMemory16(uint16_t address) {
			uint8_t lo = ReadMemory(address);
			uint8_t hi = ReadMemory(address + 1);
			return (hi << 8) | lo;
		}

		void WriteMemory16(uint16_t address, uint16_t value) {
			WriteMemory(address, value & 0xff);
			WriteMemory(address + 1, (value >> 8) & 0xff);
		}

		void RequestInterrupt(InterruptFlag interrupt) {
			WriteMemory(0xff0f, ReadMemory(0xff0f) | interrupt);
		}

		uint8_t GetHeaderChecksum() {
			uint8_t checksum = 0;
			for (uint16_t address = 0x0134; address <= 0x14c; address++) {
				checksum = checksum - m_CartridgeBank0[address] - 1;
			}

			return checksum;
		}

		uint8_t GetIF() const {
			return m_IF;
		}

		void SetIF(uint8_t value) {
			m_IF = value;
		}

		uint8_t GetIE() const {
			return m_IE;
		}

	private:
		void DisableBootROM(uint16_t addr, uint8_t value) {
			std::println("bus: disabled boot ROM access");
			m_DisableBootROM = true;
		}

		void WriteIE(uint16_t addr, uint8_t value) {
			m_IF = value;
		}

		uint8_t ReadIE(uint16_t addr) {
			return m_IF;
		}

		void WriteIF(uint16_t addr, uint8_t value) {
			m_IF = value;
		}

		uint8_t ReadIF(uint16_t addr) {
			return m_IF;
		}

	private:
		std::vector<uint8_t> m_BootROM;

		uint8_t* m_CartridgeBank0 = nullptr;
		uint8_t* m_CurrentCartridgeBank = nullptr;

		std::vector<uint8_t> m_CurrentExternalRAMBank;
		std::vector<uint8_t> m_WorkRAM;
		std::vector<uint8_t> m_HighRAM;

		uint8_t m_IE = 0;
		uint8_t m_IF = 0;

		bool m_DisableBootROM = false;
		std::shared_ptr<gb::ppu::PPU> m_PPU;
	};
}

#endif