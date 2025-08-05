#ifndef MBC3_H
#define MBC3_H

#include "base.h"

namespace pedals::mbc {
	class MBC3 : public BaseMBC {
	public:
		using BaseMBC::BaseMBC;

		MBC3(const std::vector<uint8_t>& raw, MBCFeatures features, std::optional<std::fstream>& save_file) : BaseMBC(raw, features, save_file), m_RAM(0x8000, 0) {
			if (m_SaveStream.has_value()) {
				m_SaveStream.value().read(reinterpret_cast<char*>(m_RAM.data()), 0x8000);
			}
		}

		~MBC3() {
			if (m_SaveStream.has_value()) {
				m_SaveStream.value().write(reinterpret_cast<const char*>(m_RAM.data()), m_RAM.size());
				m_SaveStream.value().close();
			}
		}

		uint8_t Read(uint16_t address) override {
			if (address <= 0x3fff) {
				return m_Raw[address];
			}

			else if (address >= 0x4000 && address <= 0x7fff) {
				return m_Raw[(address - 0x4000) + m_ROMBank * 0x4000];
			}

			else if (address >= 0xa000 && address <= 0xbfff) {
				if (!m_RAMEnabled) {
					// TODO: RTC
					return 0xff;
				}
				

				return m_RAM[(address - 0xa000) + m_RAMBank * 0x2000];
			}

			return 0xff;
		}

		void Write(uint16_t address, uint8_t value) override {
			if (address <= 0x1fff) {
				m_RAMEnabled = value == 0x0a;
			}

			else if (address >= 0x2000 && address <= 0x3fff) {
				m_ROMBank = value ? (value & 0b01111111) : 1;
			}

			else if (address >= 0x4000 && address <= 0x5fff) {
				if (value <= 0x07) {
					m_RAMBank = value;
				}
				
				else {
					m_RTCRegister = value;
				}
			}

			// TODO: RTC
			else {
				std::println("mbc3: attempted to write {:02x} -> {:04x} in rom!", value, address);
			}
		}

	private:
		uint8_t m_ROMBank = 1;
		uint8_t m_RAMBank = 0;
		uint8_t m_RTCRegister = 0;
		bool m_RAMEnabled = false;

		std::vector<uint8_t> m_RAM;
	};
}

#endif