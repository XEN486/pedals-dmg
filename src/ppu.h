#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <memory>
#include <vector>
#include <print>
#include <queue>

#define WIDTH 160
#define HEIGHT 144

namespace dmg::bus {
	class Bus;
}

namespace dmg::ppu::registers {
	enum LCDControlBits : uint8_t {
		LcdPpuEnable			= 0b10000000,
		WindowTileMapArea		= 0b01000000,
		WindowEnable			= 0b00100000,
		BgWindowTileDataArea	= 0b00010000,
		BgTileMapArea			= 0b00001000,
		ObjSize					= 0b00000100,
		ObjEnable				= 0b00000010,
		BgWindowEnable			= 0b00000001,
	};

	enum LCDStatusBits : uint8_t {
		LycIntSelect	= 0b01000000,
		Mode2IntSelect	= 0b00100000,
		Mode1IntSelect	= 0b00010000,
		Mode0IntSelect	= 0b00001000,
		LycIsLy			= 0b00000100,
		PPUMode			= 0b00000011,
	};

	template <typename T>
	class PPURegisterBase {
	public:
		PPURegisterBase() : m_Bits(static_cast<T>(0)) {}

		virtual void Set(uint8_t bits) {
			m_Bits = static_cast<T>(bits);
		}

		uint8_t Get() const {
			return m_Bits;
		}

		void SetBit(T bit) {
			Set(m_Bits | bit);
		}

		void ClearBit(T bit) {
			Set(m_Bits & ~bit);
		}

		bool GetFlag(T bit) {
			return m_Bits & bit;
		}

		uint8_t Read(uint16_t) const { return Get(); }
		void Write(uint16_t, uint8_t bits) { Set(bits); }

	protected:
		T m_Bits;
	};

	class LCDControlRegister : public PPURegisterBase<LCDControlBits> {
	public:
		bool GetLcdPpuEnabled() const {
			return m_Bits & LCDControlBits::LcdPpuEnable;
		}

		uint16_t GetWindowTileMapAreaAddress() const {
			return (m_Bits & LCDControlBits::WindowTileMapArea) ? 0x9c00 : 0x9800;
		}

		bool GetWindowEnabled() const {
			return m_Bits & LCDControlBits::WindowEnable;
		}

		uint16_t GetBgWindowTileDataAreaAddress() const {
			return (m_Bits & LCDControlBits::BgWindowTileDataArea) ? 0x8000 : 0x8800;
		}

		uint16_t GetBgTileMapAreaAddress() const {
			return (m_Bits & LCDControlBits::BgTileMapArea) ? 0x9c00 : 0x9800;
		}

		uint8_t GetObjectSize() const {
			return (m_Bits & LCDControlBits::ObjSize) ? 16 : 8;
		}

		bool GetObjectEnabled() const {
			return m_Bits & LCDControlBits::ObjEnable;
		}

		bool GetBgWindowEnabled() const {
			return m_Bits & LCDControlBits::BgWindowEnable;
		}
	};

	class LCDStatusRegister : public PPURegisterBase<LCDStatusBits> {
	public:
		void Set(uint8_t bits) override {
			m_Bits = static_cast<LCDStatusBits>((m_Bits & 0b00000111) | (bits & 0b01111000));
		}

		void SetPPUMode(uint8_t mode) {
			m_Bits = static_cast<LCDStatusBits>((m_Bits & 0b00000111) | (mode & 0b11));
		}
	};
}

namespace dmg::ppu {
	enum SpriteFlags : uint8_t {
		ObjToBgPriority = 0b10000000,
		YFlip			= 0b01000000,
		XFlip			= 0b00100000,
		OBP1			= 0b00010000,
	};

	struct Sprite {
		uint8_t y;
		uint8_t x;
		uint8_t tile;
		SpriteFlags flags;
	};

	class PPU {
	public:
		PPU() : m_VideoRAM(0x2000, 0), m_OAM(0xa0, 0), m_Frame(WIDTH * HEIGHT, 0) {}
		
		void SetBus(std::shared_ptr<dmg::bus::Bus> bus) {
			m_Bus = bus;
		}

	public:
		void WriteSCX(uint16_t, uint8_t value) {
			m_SCX = value;
		}

		void WriteSCY(uint16_t, uint8_t value) {
			m_SCY = value;
		}

		void WriteLYC(uint16_t, uint8_t value) {
			m_LYC = value;
		}

		void WriteVRAM(uint16_t address, uint8_t value) {
			m_VideoRAM[address - 0x8000] = value;
		}

		void WriteOAM(uint16_t address, uint8_t value) {
			m_OAM[address - 0xfe00] = value;
		}

		void DMATransferOAM(uint16_t, uint8_t value);

		void WriteBGP(uint16_t, uint8_t value) {
			m_BGP[3] = (value & 0b11000000) >> 6;
			m_BGP[2] = (value & 0b00110000) >> 4;
			m_BGP[1] = (value & 0b00001100) >> 2;
			m_BGP[0] = (value & 0b00000011) >> 0;
		}

		void WriteOBP0(uint16_t, uint8_t value) {
			m_OBP0[3] = (value & 0b11000000) >> 6;
			m_OBP0[2] = (value & 0b00110000) >> 4;
			m_OBP0[1] = (value & 0b00001100) >> 2;
		}

		void WriteOBP1(uint16_t, uint8_t value) {
			m_OBP1[3] = (value & 0b11000000) >> 6;
			m_OBP1[2] = (value & 0b00110000) >> 4;
			m_OBP1[1] = (value & 0b00001100) >> 2;
		}

		void WriteWX(uint16_t, uint8_t value) {
			m_WX = value;
		}

		void WriteWY(uint16_t, uint8_t value) {
			m_WY = value;
		}

	public:
		uint8_t ReadVRAM(uint16_t address) {
			return m_VideoRAM[address - 0x8000];
		}

		uint8_t ReadOAM(uint16_t address) {
			return m_OAM[address - 0xfe00];
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

		uint8_t ReadBGP(uint16_t _) {
			return (m_BGP[3] << 6) | (m_BGP[2] << 4) | (m_BGP[1] << 2) | m_BGP[0];
		}

		uint8_t ReadOBP0(uint16_t _) {
			return (m_OBP0[3] << 6) | (m_OBP0[2] << 4) | (m_OBP0[1] << 2);
		}

		uint8_t ReadOBP1(uint16_t _) {
			return (m_OBP1[3] << 6) | (m_OBP1[2] << 4) | (m_OBP1[1] << 2);
		}

		uint8_t ReadWX(uint16_t _) {
			return m_WX;
		}

		uint8_t ReadWY(uint16_t _) {
			return m_WY;
		}

	public:
		const std::vector<uint8_t>& GetFrame() {
			return m_Frame;
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

		bool ShouldRender() {
			if (!m_ShouldRender) {
				return false;
			}

			m_ShouldRender = false;
			return true;
		}

	public:
		void Update();
		void Tick();

	private:
		void RenderScanline();

	private:
		std::shared_ptr<dmg::bus::Bus> m_Bus;
		std::vector<uint8_t> m_VideoRAM;
		std::vector<uint8_t> m_OAM;

		registers::LCDControlRegister m_LCDC;
		registers::LCDStatusRegister m_STAT;

		uint8_t m_LY = 0;
		uint8_t m_LYC = 0;

		uint8_t m_SCX = 0;
		uint8_t m_SCY = 0;

		uint8_t m_LastMode = 0;
		uint8_t m_Mode = 2;

		uint8_t m_M3Duration = 172;
		size_t m_Dots = 0;

		bool m_InVBlank = false;
		bool m_ShouldRender = false;

		uint8_t m_BGP[4] = { 0 };
		uint8_t m_OBP0[4] = { 0 };
		uint8_t m_OBP1[4] = { 0 };

		uint8_t m_WX = 0;
		uint8_t m_WY = 0;

		uint8_t m_WindowLine = 0;

		std::vector<uint8_t> m_Frame;
		std::vector<Sprite> m_Sprites;
	};
}

#endif