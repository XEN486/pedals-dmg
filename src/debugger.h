#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <string>
#include <memory>

#include "bus.h"

namespace dmg::debugger {
	std::string DisassembleInstruction(std::shared_ptr<dmg::bus::Bus> bus, uint16_t pc);
}

#endif