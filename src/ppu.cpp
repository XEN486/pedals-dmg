#include "ppu.h"
#include "bus.h"

using namespace dmg::ppu;

void PPU::Tick() {
	m_Cycles++;
	switch (m_Mode) {
		case 2: {
			if (m_Cycles == 80) {
				m_Mode = 3;
			}

			break;
		}

		case 3: {
			if (m_Cycles == (172 + m_Mode3Penalty)) {
				m_Mode = 0;
			}

			break;
		}

		case 0: {
			if (m_Cycles == 456) {
				m_Cycles = 0;
				m_LY++;

				if (m_LY == 144) {
					m_ShouldRender = true;
					m_Mode = 1;
				}
				
				else {
					m_Mode = 2;
				}
			}

			break;
		}

		case 1: {
			if (m_Cycles == 456) {
				m_Cycles = 0;
				m_LY++;
				
				if (m_LY == 154) {
					m_LY = 0;
					m_Mode = 2;
				}
			}

			break;
		}
	}
}

void PPU::DMATransferOAM(uint16_t, uint8_t value) {
	for (size_t i = 0; i < 0x9f; i++) {
		m_OAM[i] = m_Bus->ReadMemory((value << 8) | i);
	}
}