#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <memory>
#include <vector>

namespace gb::bus {
	class Bus;
}

namespace gb::ppu::registers {
	enum LCDCbits : uint8_t {
		LcdPpuEnable			= 0b10000000,
		WindowTileMapArea		= 0b01000000,
		WindowEnable			= 0b00100000,
		BgWindowTileDataArea	= 0b00010000,
		BgTileMapArea			= 0b00001000,
		ObjSize					= 0b00000100,
		ObjEnable				= 0b00000010,
		BgWindowEnable			= 0b00000001,
	};

	enum STATbits : uint8_t {
		LycIntSelect	= 0b01000000,
		Mode2IntSelect	= 0b00100000,
		Mode1IntSelect	= 0b00010000,
		Mode0IntSelect	= 0b00001000,
		LycIsLy			= 0b00000100,
		PPUMode			= 0b00000011,
	};

	enum ObjectSize {
		EightByEight,
		EightBySixteen
	};

	class LCDControlRegister {
	public:
		void Set(uint16_t _, uint8_t bits) {
			Set(bits);
		}

		void Set(uint8_t bits) {
			m_Bits = static_cast<LCDCbits>(bits);
		}

		uint8_t Get(uint8_t _) const {
			return Get();
		}

		uint8_t Get() const {
			return m_Bits;
		}

		bool GetLcdPpuEnabled() const {
			return m_Bits & LCDCbits::LcdPpuEnable;
		}

		uint16_t GetWindowTileMapAreaAddress() const {
			return (m_Bits & LCDCbits::WindowTileMapArea) ? 0x9c00 : 0x9800;
		}

		bool GetWindowEnabled() const {
			return m_Bits & LCDCbits::WindowEnable;
		}

		uint16_t GetBgWindowTileDataAreaAddress() const {
			return (m_Bits & LCDCbits::BgWindowTileDataArea) ? 0x8000 : 0x8800;
		}

		uint16_t GetBgTileMapAreaAddress() const {
			return (m_Bits & LCDCbits::BgTileMapArea) ? 0x9c00 : 0x9800;
		}

		ObjectSize GetObjectSize() const {
			return (m_Bits & LCDCbits::ObjSize) ? ObjectSize::EightBySixteen : ObjectSize::EightByEight;
		}

		bool GetObjectEnabled() const {
			return m_Bits & LCDCbits::ObjEnable;
		}

		bool GetBgWindowEnabled() const {
			return m_Bits & LCDCbits::BgWindowEnable;
		}

	private:
		LCDCbits m_Bits;
	};

	class LCDStatusRegister {
	public:
		void Set(uint16_t _, uint8_t bits) {
			Set(bits);
		}

		void Set(uint8_t bits) {
			m_Bits = static_cast<STATbits>((m_Bits & 0b00000111) | (bits & 0b01111000));
		}

		uint8_t Get(uint8_t _) const {
			return Get();
		}

		uint8_t Get() const {
			return m_Bits;
		}

		uint8_t GetPPUMode() const {
			return m_Bits & STATbits::PPUMode;
		}

	private:
		STATbits m_Bits;
	};
}

namespace gb::ppu {
	class PPU {
	public:
		PPU() : m_VideoRAM(0x2000, 0) {}
		
		void SetBus(std::shared_ptr<gb::bus::Bus> bus) {
			m_Bus = bus;
		}

		uint8_t ReadLY(uint16_t _) {
			return m_LY;
		}

		uint8_t ReadLYC(uint16_t _) {
			return m_LYC;
		}

		uint8_t ReadSCX(uint16_t _) {
			return m_SCX;
		}

		uint8_t ReadSCY(uint16_t _) {
			return m_SCY;
		}

		void WriteSCX(uint16_t _, uint8_t value) {
			m_SCX = value;
		}

		void WriteSCY(uint16_t _, uint8_t value) {
			m_SCY = value;
		}

		void WriteLYC(uint16_t _, uint8_t value) {
			m_LYC = value;
		}

		void WriteVRAM(uint16_t address, uint8_t value) {
			m_VideoRAM[address - 0x8000] = value;
		}

		uint8_t ReadVRAM(uint16_t address) {
			return m_VideoRAM[address - 0x8000];
		}

		uint8_t GetSCX() {
			return m_SCX;
		}

		uint8_t GetSCY() {
			return m_SCY;
		}

		registers::LCDControlRegister& GetLCDControlRegister() {
			return m_LCDC;
		}

		registers::LCDStatusRegister& GetLCDStatusRegister() {
			return m_STAT;
		}

		bool StartedVBlank() {
			return m_Mode == 1 && m_LastMode != 1;
		}

	public:
		void Update();
		void Tick(uint8_t cycles);

	private:
		std::shared_ptr<gb::bus::Bus> m_Bus;
		std::vector<uint8_t> m_VideoRAM;

		registers::LCDControlRegister m_LCDC;
		registers::LCDStatusRegister m_STAT;

		uint8_t m_LY = 0;
		uint8_t m_LYC = 0;

		uint8_t m_SCX = 0;
		uint8_t m_SCY = 0;

		uint8_t m_LastMode = 0;
		uint8_t m_Mode = 0;

		uint8_t m_M3Duration = 172;

		size_t m_LastCycles = 0;
		size_t m_Cycles = 0;
	};
}

#endif