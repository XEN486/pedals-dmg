#ifndef BUS_H
#define BUS_H

#include "../ppu/ppu.h"
#include "../cartridge/cartridge.h"
#include "joypad.h"
#include "timer.h"

#include <stdint.h>
#include <memory>
#include <vector>
#include <print>

#include <fstream>
#include <string>
#include <cstdlib>

namespace pedals::bus {
	enum InterruptFlag : uint8_t {
		Joypad	= 0b00010000,
		Serial	= 0b00001000,
		Timer	= 0b00000100,
		LCD		= 0b00000010,
		VBlank	= 0b00000001,
	};

	class Bus {
	public:
		Bus(std::shared_ptr<pedals::ppu::PPU> ppu, std::shared_ptr<pedals::joypad::Joypad> joypad, std::shared_ptr<pedals::timer::Timer> timer, std::shared_ptr<pedals::cartridge::Cartridge> cart) :
			m_PPU(ppu),
			m_Joypad(joypad),
			m_Timer(timer),
			m_Cartridge(cart),
			m_BootROM(256, 0),
			m_WorkRAM(0x2000, 0),
			m_HighRAM(0x7f, 0) {}

		uint8_t ReadMemory(uint16_t address);
		void WriteMemory(uint16_t address, uint8_t value);

		void LoadBootROM(const std::vector<uint8_t>& rom) {
			m_BootROM = rom;
		}

		void LoadBootROM(std::string_view filename) {
			std::ifstream file(filename.data(), std::ios::binary | std::ios::ate);
			if (!file) {
				std::println("bus: failed to load boot rom file '{}'", filename);
				exit(1);
			}

			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			std::vector<uint8_t> raw(size);
			file.read(reinterpret_cast<char*>(raw.data()), size);

			LoadBootROM(raw);
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

		uint8_t GetIF() const {
			return m_IF | 0xe0;
		}

		void SetIF(uint8_t value) {
			m_IF = value;
		}

		uint8_t GetIE() const {
			return m_IE;
		}

		void SetIE(uint8_t value) {
			m_IE = value;
		}

	private:
		void DisableBootROM(uint16_t, uint8_t) {
			//std::println("bus: disabled boot ROM access");
			m_DisableBootROM = true;
		}

		uint8_t ReadSB(uint16_t) {
			return m_SB;
		}

		void WriteSB(uint16_t, uint8_t value) {
			m_SB = value;
		}

		uint8_t ReadSC(uint16_t) {
			return m_SC;
		}

		void WriteSC(uint16_t, uint8_t value) {
			m_SC = value;

			if (value == 0x81) {
				m_SC = 0;
				//printf("%c", ReadMemory(0xff01));
			}
		}

		void WriteIE(uint16_t, uint8_t value) {
			SetIE(value);
		}

		uint8_t ReadIE(uint16_t) {
			return GetIE();
		}

		void WriteIF(uint16_t, uint8_t value) {
			SetIF(value);
		}

		uint8_t ReadIF(uint16_t) {
			return GetIF();
		}

	private:
		std::vector<uint8_t> m_BootROM;

		std::vector<uint8_t> m_WorkRAM;
		std::vector<uint8_t> m_HighRAM;

		uint8_t m_IE = 0;
		uint8_t m_IF = 0;

		uint8_t m_SB = 0;
		uint8_t m_SC = 0;

		bool m_DisableBootROM = false;

		std::shared_ptr<pedals::ppu::PPU> m_PPU;
		std::shared_ptr<pedals::joypad::Joypad> m_Joypad;
		std::shared_ptr<pedals::timer::Timer> m_Timer;
		std::shared_ptr<pedals::cartridge::Cartridge> m_Cartridge;
	};
}

#endif