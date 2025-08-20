#ifndef MBC3_HPP
#define MBC3_HPP

#include "base.hpp"
#include <iostream>

namespace pedals::mbc {
	class MBC3 : public BaseMBC {
	public:
		using BaseMBC::BaseMBC;

		MBC3(const std::vector<uint8_t>& raw, MBCFeatures features, std::optional<std::fstream>& save_file)
			: BaseMBC(raw, features, save_file), m_RAM(0x8000, 0) {
			if (m_SaveStream && m_SaveStream->is_open()) {
				m_SaveStream->read(reinterpret_cast<char*>(m_RAM.data()), m_RAM.size());
			}
		}

		~MBC3() {
			if (m_SaveStream && m_SaveStream->is_open()) {
				m_SaveStream->write(reinterpret_cast<const char*>(m_RAM.data()), m_RAM.size());
				m_SaveStream->close();
			}
		}

		uint8_t Read(uint16_t address) override {
			if (address <= 0x3FFF) {
				// ROM bank 0
				return m_Raw[address];
			}

			else if (address >= 0x4000 && address <= 0x7FFF) {
				// switchable ROM bank
				size_t rom_offset = (address - 0x4000) + m_ROMBank * 0x4000;
				if (rom_offset < m_Raw.size()) {
					return m_Raw[rom_offset];
				}
				return 0xFF;
			}

			else if (address >= 0xa000 && address <= 0xbfff) {
				if (!m_RAMEnabled) return 0xff;

				if (m_UsingRTC) {
					// TODO: RTC read
					return 0xff;
				}

				if (m_RAMBank >= 4) return 0xff;
				size_t ram_offset = (address - 0xa000) + m_RAMBank * 0x2000;
				if (ram_offset < m_RAM.size()) {
					return m_RAM[ram_offset];
				}
				return 0xff;
			}

			return 0xff;
		}

		void Write(uint16_t address, uint8_t value) override {
			if (address <= 0x1fff) {
				// RAM enable
				m_RAMEnabled = (value & 0x0f) == 0x0a;
			}

			else if (address >= 0x2000 && address <= 0x3fff) {
				// ROM bank number
				value &= 0b01111111;
				m_ROMBank = (value == 0) ? 1 : value;
			}

			else if (address >= 0x4000 && address <= 0x5fff) {
				if (value <= 0x03) {
					m_RAMBank = value;
					m_UsingRTC = false;
				}
				else if (value >= 0x08 && value <= 0x0c) {
					m_RTCRegister = value;
					m_UsingRTC = true;
				}
			}

			else if (address >= 0x6000 && address <= 0x7fff) {
				// TODO: RTC latch
			}

			else if (address >= 0xa000 && address <= 0xbfff) {
				if (!m_RAMEnabled) return;

				if (m_UsingRTC) {
					// TODO: RTC write
					return;
				}

				if (m_RAMBank >= 4) return;
				size_t ram_offset = (address - 0xa000) + m_RAMBank * 0x2000;
				if (ram_offset < m_RAM.size()) {
					m_RAM[ram_offset] = value;
				}
			}

			else {
				std::println("mbc3: attempted to write {:02x} -> {:04x} in rom!", value, address);
			}
		}

	private:
		uint8_t m_ROMBank = 1;
		uint8_t m_RAMBank = 0;
		uint8_t m_RTCRegister = 0;
		bool m_UsingRTC = false;
		bool m_RAMEnabled = false;

		std::vector<uint8_t> m_RAM;
	};
}

#endif