#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "bus.h"

#include <stdint.h>
#include <string>
#include <memory>

namespace gb::cartridge {
	class Cartridge {
	public:
		// load banks 0 and 1 by default.
		Cartridge(std::shared_ptr<gb::bus::Bus> bus, std::string_view filename) : m_Bus(bus), m_Filename(filename) {
			ParseFile();
			LoadBank(0);
			LoadBank(1);
		}

		void ParseFile();
		void LoadBank(uint8_t bank);

	private:
		std::vector<uint8_t> m_Raw;
		std::shared_ptr<gb::bus::Bus> m_Bus;
		std::string m_Filename;
	};
}

#endif