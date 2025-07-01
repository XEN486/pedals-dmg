#include "bus.h"
#include <print>

using namespace gb::bus;

#define RouteRange(start, end) if (address >= start && address <= end)
#define RouteRead(addr, func) if (address == addr) { return func(addr); }
#define RouteWrite(addr, value, func) if (address == addr) { func(addr, value); return; }

uint8_t Bus::ReadMemory(uint16_t address) {
	if (address >= 0x0000 && address <= 0x00ff) {
		if (m_DisableBootROM) {
			return m_CartridgeBank0[address];
		}
		
		return m_BootROM[address];
	}
	
	RouteRange(0x00ff, 0x3fff) {
		return m_CartridgeBank0[address];
	}

	RouteRange(0x4000, 0x7fff) {
		return m_CurrentCartridgeBank[address - 0x4000];
	}

	RouteRange(0x8000, 0x9fff) {
		return m_PPU->ReadVRAM(address);
	}

	RouteRange(0xa000, 0xbfff) {
		return m_CurrentExternalRAMBank[address - 0xa000];
	}

	RouteRange(0xc000, 0xdfff) {
		return m_WorkRAM[address - 0xc000];
	}

	RouteRange(0xe000, 0xfdff) {
		return m_WorkRAM[address - 0xe000];
	}

	RouteRange(0xff80, 0xfffe) {
		return m_HighRAM[address - 0xff80];
	}

	// PPU registers
	RouteRead(0xff40, m_PPU->GetLCDControlRegister().Get);
	RouteRead(0xff41, m_PPU->GetLCDStatusRegister().Get);
	RouteRead(0xff42, m_PPU->ReadSCY);
	RouteRead(0xff43, m_PPU->ReadSCX);
	RouteRead(0xff44, m_PPU->ReadLY);
	RouteRead(0xff45, m_PPU->ReadLYC);
	RouteRead(0xff47, m_PPU->ReadBGP);
	RouteRead(0xff48, m_PPU->ReadOBP0);
	RouteRead(0xff49, m_PPU->ReadOBP1);

	// interrupts
	RouteRead(0xff0f, ReadIF);
	RouteRead(0xffff, ReadIE);

	std::println("bus: attempted to read from unknown address {:x}", address);
	return 0;
}

void Bus::WriteMemory(uint16_t address, uint8_t value) {
	RouteRange(0x0000, 0x00ff) {
		std::println("bus: attempted to write {:x} -> {:x} in rom!", value, address);
		return;
	}
	
	RouteRange(0x00ff, 0x3fff) {
		std::println("bus: attempted to write {:x} -> {:x} in rom!", value, address);
		return;
	}

	RouteRange(0x4000, 0x7fff) {
		std::println("bus: attempted to write {:x} -> {:x} in rom!", value, address);
		return;
	}

	RouteRange(0x8000, 0x9fff) {
		m_PPU->WriteVRAM(address, value);
		return;
	}

	RouteRange(0xa000, 0xbfff) {
		m_CurrentExternalRAMBank[address - 0xa000] = value;
		return;
	}

	RouteRange(0xc000, 0xdfff) {
		m_WorkRAM[address - 0xc000] = value;
		return;
	}

	RouteRange(0xe000, 0xfdff) {
		m_WorkRAM[address - 0xe000] = value;
		return;
	}
	
	RouteRange(0xff80, 0xfffe) {
		m_HighRAM[address - 0xff80] = value;
		return;
	}

	// I/O registers
	RouteWrite(0xff50, value, DisableBootROM);

	// PPU registers
	RouteWrite(0xff40, value, m_PPU->GetLCDControlRegister().Set)
	RouteWrite(0xff41, value, m_PPU->GetLCDStatusRegister().Set);
	RouteWrite(0xff42, value, m_PPU->WriteSCY);
	RouteWrite(0xff43, value, m_PPU->WriteSCX);
	RouteWrite(0xff45, value, m_PPU->WriteLYC);
	RouteWrite(0xff47, value, m_PPU->WriteBGP);
	RouteWrite(0xff48, value, m_PPU->WriteOBP0);
	RouteWrite(0xff49, value, m_PPU->WriteOBP1);

	// interrupt stuff
	RouteWrite(0xff0f, value, WriteIF);
	RouteWrite(0xffff, value, WriteIE);

	std::println("bus: attempted to write {:x} -> unknown address {:x}", value, address);
}