#include "cartridge.hpp"
#include <cstdlib>
#include <fstream>

using namespace pedals::cartridge;

void Cartridge::ParseFile() {
	std::ifstream file(m_Filename, std::ios::binary | std::ios::ate);
	if (!file) {
		std::println("could not load file '{}'", m_Filename);
		exit(1);
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	m_Raw.resize(size);
	file.read(reinterpret_cast<char*>(m_Raw.data()), size);
}