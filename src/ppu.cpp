#include "ppu.h"
#include "bus.h"
using namespace gb::ppu;

// request interrupt if a mode changed
#define ModeInterrupt(flag, mode) \
	if ((m_STAT.Get() & flag) != 0 && m_Mode == mode && m_LastMode != mode) { \
		m_Bus->RequestInterrupt(gb::bus::InterruptFlag::LCD); \
	}

void PPU::Update() {
	if (m_LY == m_LYC) {
		// set LYC == LY bit
		m_STAT.Set(m_STAT.Get() | registers::STATbits::LycIsLy);

		// if LYC interrupt is enabled then request the interrupt in IF
		if (m_STAT.Get() & registers::STATbits::LycIntSelect) {
			//std::println("ppu: LY == LYC interrupt (LY = {})", m_LY);
			m_Bus->RequestInterrupt(gb::bus::InterruptFlag::LCD);
		}
	}

	ModeInterrupt(registers::STATbits::Mode0IntSelect, 0);
	ModeInterrupt(registers::STATbits::Mode1IntSelect, 1);
	ModeInterrupt(registers::STATbits::Mode2IntSelect, 2);

	// set the PPU mode
	m_STAT.Set((m_STAT.Get() & 0b11111100) | (m_Mode & 0b11));
}

void PPU::Tick(uint8_t cycles) {
	m_LastCycles = cycles;
	m_Cycles += cycles;

	m_LastMode = m_Mode;
	switch (m_Mode) {
		// Horizontal blank
		case 0: {
			if (m_Cycles < (376 - m_M3Duration)) {
				break;
			}

			m_Cycles = 0;
			m_LY++;

			if (m_LY == 144) {
				m_Mode = 1;
			} else {
				m_Mode = 2;
			}

			break;
		}

		// OAM scan
		case 2: {
			if (m_Cycles < 80) {
				break;
			}

			m_Mode = 3;
			break;
		}

		// Drawing pixels
		case 3: {
			if (m_Cycles < (80 + m_M3Duration)) {
				break;
			}

			m_Mode = 0;
			break;
		}

		// Vertical blank
		case 1: {
			m_Cycles += cycles;

			if (m_Cycles >= 456) {
				m_Cycles -= 456;
				m_LY++;

				if (m_LY == 144 && m_Mode != 1) {
					m_Bus->RequestInterrupt(gb::bus::InterruptFlag::VBlank);
				}

				if (m_LY > 153) {
					m_LY = 0;
					m_Mode = 2;
				}
			}
			break;
		}

		default: {
			std::println("ppu: invalid mode {}! going to mode 0..", m_Mode);
			m_Mode = 0;
			break;
		}
	}

	Update();
}