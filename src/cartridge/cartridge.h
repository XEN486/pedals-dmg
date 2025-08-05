#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "mbc/base.h"
#include "mbc/mbc1.h"
#include "mbc/mbc3.h"

#include <stdint.h>
#include <string>
#include <memory>
#include <vector>
#include <print>
#include <filesystem>

namespace pedals::cartridge {
	class Cartridge {
	public:
		Cartridge(std::string_view filename) : m_Filename(filename) {
			ParseFile();

			pedals::mbc::MBCFeatures features = pedals::mbc::get_mbc_features(m_Raw[0x147]);
			std::optional<std::fstream> stream;

			if (features.ram && features.battery) {
				std::string save_filename = std::string(filename.substr(0, filename.find_last_of('.'))) + ".sav";
				bool new_save = !std::filesystem::exists(save_filename);

				if (new_save) {
					std::ofstream init_save(save_filename, std::ios::binary);
					std::vector<char> zero(0x8000, 0);
					init_save.write(zero.data(), zero.size());
				}

				stream.emplace(save_filename, std::ios::in | std::ios::out | std::ios::binary);
				if (!stream->is_open()) {
					std::println("cartridge: failed to open save file");
				}
			}
			
			switch (features.mbc) {
				case pedals::mbc::MBCType::MBC1: m_MBC = new pedals::mbc::MBC1(m_Raw, features, stream); break;
				case pedals::mbc::MBCType::MBC3: m_MBC = new pedals::mbc::MBC3(m_Raw, features, stream); break;
				case pedals::mbc::MBCType::ROM: m_MBC = new pedals::mbc::NoMBC(m_Raw, features, stream); break;

				default: {
					std::println("cartridge: mbc type {:02x} is unimplemented", m_Raw[0x147]);
					exit(1);
				}
			}
		}

		~Cartridge() {
			delete m_MBC;
		}

		pedals::mbc::BaseMBC* GetMBC() {
			return m_MBC;
		}

		void ParseFile();

	private:
		std::vector<uint8_t> m_Raw;
		std::string m_Filename;
		pedals::mbc::BaseMBC* m_MBC;
	};
}

#endif