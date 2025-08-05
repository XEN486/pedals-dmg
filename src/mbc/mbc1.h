#ifndef MBC1_H
#define MBC1_H

#include "base.h"

namespace dmg::mbc {
	class MBC1 : public BaseMBC {
	public:
		using BaseMBC::BaseMBC;

		MBC1(const std::vector<uint8_t>& raw, MBCFeatures features, std::optional<std::fstream>& save_file) : BaseMBC(raw, features, save_file), m_RAM(0x8000, 0) {
			if (m_SaveStream.has_value()) {
				m_SaveStream.value().read(reinterpret_cast<char*>(m_RAM.data()), 0x8000);
			}
		}

		~MBC1() {
			if (m_SaveStream.has_value()) {
				m_SaveStream.value().write(reinterpret_cast<const char*>(m_RAM.data()), m_RAM.size());
				m_SaveStream.value().close();
			}
		}

		uint8_t Read(uint16_t address) override {
			if (address < 0x4000) {
				if (m_BankingMode == 1 && m_Raw.size() > 0x80000) {
					uint32_t bank = m_ROMBank2 << 5;
					return m_Raw[address + bank * 0x4000];
				}
				return m_Raw[address];
			}

			else if (address < 0x8000) {
				uint32_t bank = m_ROMBank | (m_ROMBank2 << 5);
				return m_Raw[(address - 0x4000) + bank * 0x4000];
			}

			else if (address >= 0xa000 && address < 0xc000) {
				if (!m_RAMEnabled) return 0xff;
				uint16_t ramAddr = (address - 0xa000) + (m_BankingMode ? m_RAMBank * 0x2000 : 0);
				return m_RAM[ramAddr];
			}

			return 0xff;
		}

		void Write(uint16_t address, uint8_t value) override {
			if (address < 0x2000) {
				m_RAMEnabled = ((value & 0x0f) == 0x0a) && m_Features.ram;
			}
			
			else if (address < 0x4000) {
				uint8_t bank = value & 0b00011111;
				m_ROMBank = (bank == 0) ? 1 : bank;
			}
			
			else if (address < 0x6000) {
				if (m_BankingMode == 0) {
					m_ROMBank2 = value & 0b11;
				} else {
					m_RAMBank = value & 0b11;
				}
			}
			
			else if (address < 0x8000) {
				m_BankingMode = value & 1;
			}
			
			else if (address >= 0xa000 && address < 0xc000) {
				if (!m_RAMEnabled) return;
				uint16_t ramAddr = (address - 0xa000) + (m_BankingMode ? m_RAMBank * 0x2000 : 0);
				m_RAM[ramAddr] = value;
			}

			else {
				std::println("mbc1: attempted to write {:02x} -> {:04x} in rom!", value, address);
			}
		}

	private:
		uint8_t m_ROMBank = 1;
		uint8_t m_ROMBank2 = 0;
		uint8_t m_RAMBank = 0;
		uint8_t m_BankingMode = 0;
		bool m_RAMEnabled = false;

		std::vector<uint8_t> m_RAM;
	};
}

#endif