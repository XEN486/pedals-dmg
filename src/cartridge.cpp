#include "cartridge.h"
#include <fstream>

using namespace gb::cartridge;

void Cartridge::ParseFile() {
	std::ifstream file(m_Filename, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	m_Raw.resize(size);
	file.read(reinterpret_cast<char*>(m_Raw.data()), size);
}

void Cartridge::LoadBank(uint8_t bank) {
	std::size_t offset = bank * 0x4000;
	uint8_t* bank_data = m_Raw.data() + offset;

	if (bank == 0) {
		m_Bus->LoadCartridgeBank0(bank_data);
	} else {
		m_Bus->LoadCartridgeBank(bank_data);
	}
}