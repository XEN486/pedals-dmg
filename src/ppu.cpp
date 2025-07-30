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
			if (m_Cycles == 81) {
				RenderScanline();
			}

			else if (m_Cycles == (80 + 172 + m_Mode3Penalty)) {
				m_Mode = 0;

				if (m_STAT.GetFlag(registers::LCDStatusBits::Mode0IntSelect)) {
					m_Bus->RequestInterrupt(dmg::bus::InterruptFlag::LCD);
				}
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

					m_Bus->RequestInterrupt(dmg::bus::InterruptFlag::VBlank);

					if (m_STAT.GetFlag(registers::LCDStatusBits::Mode1IntSelect)) {
						m_Bus->RequestInterrupt(dmg::bus::InterruptFlag::LCD);
					}
				}
				
				else {
					m_Mode = 2;

					if (m_STAT.GetFlag(registers::LCDStatusBits::Mode2IntSelect)) {
						m_Bus->RequestInterrupt(dmg::bus::InterruptFlag::LCD);
					}
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

	// if LYC == LY then set the bit and request an interrupt
	if (m_LYC == m_LY) {
		m_STAT.SetWithoutMask(m_STAT.Get() | 0b00000100);

		if (m_STAT.GetFlag(registers::LCDStatusBits::LycIntSelect)) {
			m_Bus->RequestInterrupt(dmg::bus::InterruptFlag::LCD);
		}
	}
	
	// clear LYC == LY
	else {
		m_STAT.SetWithoutMask(m_STAT.Get() & 0b11111011);
	}

	// PPU mode
	m_STAT.SetWithoutMask((m_STAT.Get() & 0b11111100) | (m_Mode & 0b00000011));
}

void PPU::DMATransferOAM(uint16_t, uint8_t value) {
	for (size_t i = 0; i < 0x9f; i++) {
		m_OAM[i] = m_Bus->ReadMemory((value << 8) | i);
	}
}

void PPU::RenderScanline() {
	uint8_t bg_y = (m_SCY + m_LY) & 0xff;
	uint8_t bg_tile_row = bg_y / 8;
	uint8_t bg_pixel_y = bg_y % 8;

	
	for (int x = 0; x < WIDTH; x++) {
		uint8_t bg_color_index = 0;

		// background
		if (m_LCDC.GetFlag(registers::LCDControlBits::BgWindowEnable)) {
			uint8_t bg_x = (m_SCX + x) & 0xff;
			uint8_t tile_col = bg_x / 8;
			uint8_t pixel_x = bg_x % 8;

			uint16_t map_address = (m_LCDC.GetFlag(registers::LCDControlBits::BgTileMapArea) ? 0x9c00 : 0x9800) + bg_tile_row * 32 + tile_col;
			uint8_t tile_index = ReadVRAM(map_address);

			uint16_t tile_data_base = m_LCDC.GetFlag(registers::LCDControlBits::BgWindowTileDataArea) ? 0x8000 : 0x9000;
			int tile_number = (tile_data_base == 0x9000) ? static_cast<int8_t>(tile_index) : tile_index;
			uint16_t tile_address = tile_data_base + tile_number * 16;

			uint8_t low_byte = ReadVRAM(tile_address + bg_pixel_y * 2);
			uint8_t high_byte = ReadVRAM(tile_address + bg_pixel_y * 2 + 1);

			uint8_t bit_index = 7 - pixel_x;
			bg_color_index = ((high_byte >> bit_index) & 1) << 1 | ((low_byte >> bit_index) & 1);
		}

		m_Frame[m_LY * WIDTH + x] = m_BGP[bg_color_index];
	}
}