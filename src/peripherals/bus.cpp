#include "bus.h"
#include <print>

using namespace dmg::bus;

#define RouteRange(start, end) if (address >= start && address <= end)
#define RouteRead(addr, func) if (address == addr) { return func(addr); }
#define RouteWrite(addr, func) if (address == addr) { func(addr, value); return; }

uint8_t Bus::ReadMemory(uint16_t address) {
	if (address >= 0x0000 && address <= 0x00ff) {
		if (m_DisableBootROM) {
			return m_Cartridge->GetMBC()->Read(address);
		}
		
		return m_BootROM[address];
	}

	RouteRange(0x00ff, 0x7fff) {
		return m_Cartridge->GetMBC()->Read(address);
	}

	RouteRange(0x8000, 0x9fff) {
		return m_PPU->ReadVRAM(address);
	}

	RouteRange(0xa000, 0xbfff) {
		return m_Cartridge->GetMBC()->Read(address);
	}

	RouteRange(0xc000, 0xcfff) {
		return m_WorkRAM[address - 0xc000];
	}

	RouteRange(0xd000, 0xdfff) {
		return m_WorkRAM[address - 0xc000];
	}

	RouteRange(0xe000, 0xfdff) {
		return m_WorkRAM[address - 0xe000];
	}

	RouteRange(0xff80, 0xfffe) {
		return m_HighRAM[address - 0xff80];
	}

	// not usable
	RouteRange(0xfea0, 0xfeff) {
		return 0xff;
	}

	// PPU OAM
	RouteRange(0xfe00, 0xfe9f) {
		return m_PPU->ReadOAM(address);
	}

	// Joypad
	RouteRead(0xff00, m_Joypad->ReadP1);

	// Serial
	RouteRead(0xff01, ReadSB);
	RouteRead(0xff02, ReadSC);

	// Timer
	RouteRead(0xff04, m_Timer->ReadDIV);
	RouteRead(0xff05, m_Timer->ReadTIMA);
	RouteRead(0xff06, m_Timer->ReadTMA);
	RouteRead(0xff07, m_Timer->ReadTAC);

	// PPU registers
	RouteRead(0xff40, m_PPU->GetLCDControlRegister().Read);
	RouteRead(0xff41, m_PPU->GetLCDStatusRegister().Read);
	RouteRead(0xff42, m_PPU->ReadSCY);
	RouteRead(0xff43, m_PPU->ReadSCX);
	RouteRead(0xff44, m_PPU->ReadLY);
	RouteRead(0xff45, m_PPU->ReadLYC);
	RouteRead(0xff47, m_PPU->ReadBGP);
	RouteRead(0xff48, m_PPU->ReadOBP0);
	RouteRead(0xff49, m_PPU->ReadOBP1);
	RouteRead(0xff4a, m_PPU->ReadWX);
	RouteRead(0xff4b, m_PPU->ReadWY);

	// suppress unknown APU registers because it will not be implemented for now
	RouteRange(0xff10, 0xff3f) {
		return 0;
	}

	// unused CGB registers
	RouteRange(0xff4c, 0xff7f) {
		return 0xff;
	}

	// interrupts
	RouteRead(0xff0f, ReadIF);
	RouteRead(0xffff, ReadIE);

	std::println("bus: attempted to read from unknown address {:x}", address);
	return 0xff;
}

void Bus::WriteMemory(uint16_t address, uint8_t value) {
	RouteRange(0x0000, 0x00ff) {
		if (!m_DisableBootROM) {
			std::println("bus: attempted to write {:x} -> {:x} in boot rom!", value, address);
		}

		m_Cartridge->GetMBC()->Write(address, value);
		return;
	}
	
	RouteRange(0x00ff, 0x7fff) {
		m_Cartridge->GetMBC()->Write(address, value);
		return;
	}

	RouteRange(0x8000, 0x9fff) {
		m_PPU->WriteVRAM(address, value);
		return;
	}

	RouteRange(0xa000, 0xbfff) {
		m_Cartridge->GetMBC()->Write(address, value);
		return;
	}

	RouteRange(0xc000, 0xcfff) {
		m_WorkRAM[address - 0xc000] = value;
		return;
	}

	RouteRange(0xd000, 0xdfff) {
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

	// not usable
	RouteRange(0xfea0, 0xfeff) {
		return;
	}

	// PPU OAM
	RouteRange(0xfe00, 0xfe9f) {
		return m_PPU->WriteOAM(address, value);
	}

	// Serial
	RouteWrite(0xff01, WriteSB);
	RouteWrite(0xff02, WriteSC);
	
	// Timer
	RouteWrite(0xff04, m_Timer->WriteDIV);
	RouteWrite(0xff05, m_Timer->WriteTIMA);
	RouteWrite(0xff06, m_Timer->WriteTMA);
	RouteWrite(0xff07, m_Timer->WriteTAC);

	// Joypad
	RouteWrite(0xff00, m_Joypad->WriteP1);

	// PPU registers
	RouteWrite(0xff40, m_PPU->GetLCDControlRegister().Write)
	RouteWrite(0xff41, m_PPU->GetLCDStatusRegister().Write);
	RouteWrite(0xff42, m_PPU->WriteSCY);
	RouteWrite(0xff43, m_PPU->WriteSCX);
	RouteWrite(0xff45, m_PPU->WriteLYC);
	RouteWrite(0xff46, m_PPU->DMATransferOAM);
	RouteWrite(0xff47, m_PPU->WriteBGP);
	RouteWrite(0xff48, m_PPU->WriteOBP0);
	RouteWrite(0xff49, m_PPU->WriteOBP1);
	RouteWrite(0xff4a, m_PPU->WriteWX);
	RouteWrite(0xff4b, m_PPU->WriteWY);

	// I/O registers
	RouteWrite(0xff50, DisableBootROM);

	// unused CGB registers
	RouteRange(0xff4c, 0xff7f) {
		return;
	}

	// suppress unknown APU registers because it will not be implemented for now
	RouteRange(0xff10, 0xff3f) {
		return;
	}

	// interrupt stuff
	RouteWrite(0xff0f, WriteIF);
	RouteWrite(0xffff, WriteIE);

	std::println("bus: attempted to write {:x} -> unknown address {:x}", value, address);
}