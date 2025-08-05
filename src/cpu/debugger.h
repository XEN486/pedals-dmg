#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <string>
#include <memory>

#include "../peripherals/bus.h"

namespace pedals::debugger {
	std::string DisassembleInstruction(std::shared_ptr<pedals::bus::Bus> bus, uint16_t pc);
}

#endif