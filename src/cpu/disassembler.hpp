#ifndef DISASSEMBLER_HPP
#define DISASSEMBLER_HPP

#include <string>
#include <memory>

#include "../peripherals/bus.hpp"

namespace pedals::cpu {
	std::string DisassembleInstruction(std::shared_ptr<pedals::bus::Bus> bus, uint16_t pc);
}

#endif