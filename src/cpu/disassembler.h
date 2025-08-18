#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <string>
#include <memory>

#include "../peripherals/bus.h"

namespace pedals::cpu {
	std::string DisassembleInstruction(std::shared_ptr<pedals::bus::Bus> bus, uint16_t pc);
}

#endif