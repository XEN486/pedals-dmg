#ifndef MBC_BASE_H
#define MBC_BASE_H

#include <stdint.h>
#include <vector>
#include <print>
#include <optional>
#include <fstream>

namespace pedals::mbc {
	enum class MBCType {
		ROM,
		MBC1,
		MBC2,
		MMM01,
		MBC3,
		MBC5,
		MBC6,
		MBC7,
		TAMA5,
		HuC3,
		HuC1,
		Other,
	};

	struct MBCFeatures {
		MBCType mbc;
		bool ram;
		bool battery;
		bool timer;
		bool rumble;
		bool sensor;
		bool camera;	
	};

	static MBCFeatures get_mbc_features(uint8_t byte) {
		switch (byte) {
			//					MBC name		RAM		BATTERY	TIMER	RUMBLE	SENSOR	CAMERA
			case 0x00: return { MBCType::ROM,	false,	false,	false,	false,	false,	false };
			case 0x01: return { MBCType::MBC1,	false,	false,	false,	false,	false,	false };
			case 0x02: return { MBCType::MBC1,	true,	false,	false,	false,	false,	false };
			case 0x03: return { MBCType::MBC1,	true,	true,	false,	false,	false,	false };
			case 0x05: return { MBCType::MBC2,	false,	false,	false,	false,	false,	false };
			case 0x06: return { MBCType::MBC2,	false,	true,	false,	false,	false,	false };
			case 0x08: return { MBCType::ROM,	true,	false,	false,	false,	false,	false };
			case 0x09: return { MBCType::ROM,	true,	true,	false,	false,	false,	false };
			case 0x0b: return { MBCType::MMM01,	false,	false,	false,	false,	false,	false };
			case 0x0c: return { MBCType::MMM01,	true,	false,	false,	false,	false,	false };
			case 0x0d: return { MBCType::MMM01,	true,	true,	false,	false,	false,	false };
			case 0x0f: return { MBCType::MBC3,	false,	true,	true,	false,	false,	false };
			case 0x10: return { MBCType::MBC3,	true,	true,	true,	false,	false,	false };
			case 0x11: return { MBCType::MBC3,	false,	false,	false,	false,	false,	false };
			case 0x12: return { MBCType::MBC3,	true,	false,	false,	false,	false,	false };
			case 0x13: return { MBCType::MBC3,	true,	true,	false,	false,	false,	false };
			case 0x19: return { MBCType::MBC5,	false,	false,	false,	false,	false,	false };
			case 0x1a: return { MBCType::MBC5,	true,	false,	false,	false,	false,	false };
			case 0x1b: return { MBCType::MBC5,	true,	true,	false,	false,	false,	false };
			case 0x1c: return { MBCType::MBC5,	false,	false,	false,	true,	false,	false };
			case 0x1d: return { MBCType::MBC5,	true,	false,	false,	true,	false,	false };
			case 0x1e: return { MBCType::MBC5,	true,	true,	false,	true,	false,	false };
			case 0x20: return { MBCType::MBC6,	false,	false,	false,	false,	false,	false };
			case 0x22: return { MBCType::MBC7,	true,	true,	false,	true,	true,	false };
			case 0xfc: return {	MBCType::Other,	false,	false,	false,	false,	false,	true  };
			case 0xfd: return { MBCType::TAMA5,	false,	false,	false,	false,	false,	false };
			case 0xfe: return { MBCType::HuC3,	false,	false,	false,	false,	false,	false };
			case 0xff: return { MBCType::HuC1,	true,	true,	false,	false,	false,	false };
			
			default: {
				std::println("mbc: unknown mbc type {:02x}", byte);
				return { MBCType::ROM, false, false, false, false, false, false };
			}
		}
	}

	class BaseMBC {
	public:
		BaseMBC(const std::vector<uint8_t>& raw, MBCFeatures features, std::optional<std::fstream>& save_file)
			: m_Raw(raw), m_Features(features), m_SaveStream(save_file) {}

		virtual uint8_t Read(uint16_t) = 0;
		virtual void Write(uint16_t, uint8_t) = 0;

	protected:
		const std::vector<uint8_t>& m_Raw;
		MBCFeatures m_Features;
		std::optional<std::fstream>& m_SaveStream;
	};

	class NoMBC : public BaseMBC {
	public:
		using BaseMBC::BaseMBC;
		
		// TODO: make this check if the address is valid
		uint8_t Read(uint16_t address) override {
			return m_Raw[address];
		}

		void Write(uint16_t address, uint8_t value) override {
			std::println("mbc: attempted to write {:02x} -> {:04x} in rom!", value, address);
		}
	};
}

#endif