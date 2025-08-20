#include "ppu.hpp"
#include "../peripherals/bus.hpp"

#include <algorithm>
using namespace pedals::ppu;

void PPU::Tick() {
	switch (m_Mode) {
		case 2: {
			if (m_Dots == 0) {
				m_Sprites.clear();
			}

			if (m_Dots < 80 && m_Dots % 2 == 0) {
				size_t sprite_index = m_Dots / 2;
				
				if (sprite_index < 40) {
					size_t oam_index = sprite_index * 4;

					uint8_t y = m_OAM[oam_index];
					uint8_t x = m_OAM[oam_index + 1];
					uint8_t tile = m_OAM[oam_index + 2];
					SpriteFlags flags = static_cast<SpriteFlags>(m_OAM[oam_index + 3]);

					int sprite_height = m_LCDC.GetFlag(registers::LCDControlBits::ObjSize) ? 16 : 8;
					uint8_t sprite_y = y - 16;

					if (m_LY >= sprite_y && m_LY < sprite_y + sprite_height && m_Sprites.size() < 10) {
						m_Sprites.emplace_back(y, x, tile, flags, sprite_index);
					}
				}
			}

			if (m_Dots == 80) {
				m_Mode = 3;
			}

			break;
		}

		case 3: {
			if (m_Dots == 81) {
				RenderScanline();
			}

			else if (m_Dots == (80 + 172 + m_Mode3Penalty)) {
				m_Mode = 0;

				if (m_STAT.GetFlag(registers::LCDStatusBits::Mode0IntSelect)) {
					m_Bus->RequestInterrupt(pedals::bus::InterruptFlag::LCD);
				}
			}

			break;
		}

		case 0: {
			if (m_Dots == 456) {
				m_Dots = -1;
				m_LY++;

				if (m_LY == 144) {
					m_ShouldRender = true;
					m_Mode = 1;

					m_Bus->RequestInterrupt(pedals::bus::InterruptFlag::VBlank);

					if (m_STAT.GetFlag(registers::LCDStatusBits::Mode1IntSelect)) {
						m_Bus->RequestInterrupt(pedals::bus::InterruptFlag::LCD);
					}
				}
				
				else {
					m_Mode = 2;

					if (m_STAT.GetFlag(registers::LCDStatusBits::Mode2IntSelect)) {
						m_Bus->RequestInterrupt(pedals::bus::InterruptFlag::LCD);
					}
				}
			}

			break;
		}

		case 1: {
			if (m_Dots == 456) {
				m_Dots = -1;
				m_LY++;

				if (m_LY == 154) {
					m_LY = 0;
					m_Mode = 2;
					m_WindowLineResetPending = true;
				}
			}

			break;
		}
	}

	// if LYC == LY then set the bit and request an interrupt
	if (m_LYC == m_LY && !m_DontCheckLYC) {
		m_STAT.SetWithoutMask(m_STAT.Get() | 0b00000100);

		if (m_STAT.GetFlag(registers::LCDStatusBits::LycIntSelect)) {
			m_Bus->RequestInterrupt(pedals::bus::InterruptFlag::LCD);
		}

		m_DontCheckLYC = true;
	}
	
	// clear LYC == LY
	else if (m_LYC != m_LY) {
		m_STAT.SetWithoutMask(m_STAT.Get() & 0b11111011);
		m_DontCheckLYC = false;
	}

	// PPU mode
	m_STAT.SetWithoutMask((m_STAT.Get() & 0b11111100) | (m_Mode & 0b00000011));

	m_Dots++;
}

void PPU::DMATransferOAM(uint16_t, uint8_t value) {
	for (uint16_t i = 0; i < 0xa0; ++i) {
		m_OAM[i] = m_Bus->ReadMemory((static_cast<uint16_t>(value) << 8) | i);
	}
}

void PPU::RenderScanline() {
	if (m_LY >= 144) return;

	uint8_t bg_y = static_cast<uint8_t>((m_SCY + m_LY) & 0xff);
	uint8_t bg_tile_row = bg_y / 8;
	uint8_t bg_pixel_y = bg_y % 8;

	int wx = static_cast<int>(m_WX) - 7;
	bool window_enabled = m_LCDC.GetFlag(registers::LCDControlBits::WindowEnable);
	bool window_visible_this_line = window_enabled && (wx < WIDTH) && (m_LY >= m_WY);

	if (window_visible_this_line && m_WindowLineResetPending) {
		m_WindowLine = 0;
		m_WindowLineResetPending = false;
	}

	for (int x = 0; x < WIDTH; ++x) {
		uint8_t bg_window_color = 0;

		if (m_LCDC.GetFlag(registers::LCDControlBits::BgWindowEnable)) {
			// window pixel
			if (window_visible_this_line && x >= wx && false) {
				int window_x = x - wx;
				uint8_t window_tile_row = m_WindowLine / 8;
				uint8_t window_pixel_y = m_WindowLine % 8;

				int tile_col = window_x / 8;
				int pixel_x = window_x % 8;

				uint16_t map_base = m_LCDC.GetFlag(registers::LCDControlBits::WindowTileMapArea) ? 0x9c00 : 0x9800;
				uint16_t map_addr = static_cast<uint16_t>(map_base + window_tile_row * 32 + tile_col);
				uint8_t tile_index = ReadVRAM(map_addr);

				uint16_t data_base = m_LCDC.GetFlag(registers::LCDControlBits::BgWindowTileDataArea) ? 0x8000 : 0x9000;
				int tile_number = (data_base == 0x9000) ? static_cast<int8_t>(tile_index) : tile_index;
				uint16_t tile_addr = static_cast<uint16_t>(data_base + tile_number * 16);

				uint8_t low_byte  = ReadVRAM(tile_addr + window_pixel_y * 2);
				uint8_t high_byte = ReadVRAM(tile_addr + window_pixel_y * 2 + 1);

				int bit_index = 7 - pixel_x;
				bg_window_color = static_cast<uint8_t>(((high_byte >> bit_index) & 1) << 1 | ((low_byte >> bit_index) & 1));
			}

			// background pixel
			else {
				int bg_x = (m_SCX + x) & 0xff;
				int tile_col = bg_x / 8;
				int pixel_x = bg_x % 8;

				uint16_t map_base = m_LCDC.GetFlag(registers::LCDControlBits::BgTileMapArea) ? 0x9c00 : 0x9800;
				uint16_t map_addr = static_cast<uint16_t>(map_base + bg_tile_row * 32 + tile_col);
				uint8_t tile_index = ReadVRAM(map_addr);

				uint16_t data_base = m_LCDC.GetFlag(registers::LCDControlBits::BgWindowTileDataArea) ? 0x8000 : 0x9000;
				int tile_number = (data_base == 0x9000) ? static_cast<int8_t>(tile_index) : tile_index;
				uint16_t tile_addr = static_cast<uint16_t>(data_base + tile_number * 16);

				uint8_t low_byte = ReadVRAM(tile_addr + bg_pixel_y * 2);
				uint8_t high_byte = ReadVRAM(tile_addr + bg_pixel_y * 2 + 1);

				int bit_index = 7 - pixel_x;
				bg_window_color = static_cast<uint8_t>(((high_byte >> bit_index) & 1) << 1 | ((low_byte >> bit_index) & 1));
			}

			m_Frame[m_LY * WIDTH + x] = m_BGP[bg_window_color];
		}

		else {
			m_Frame[m_LY * WIDTH + x] = 0;
		}

		// sprites
		if (m_LCDC.GetFlag(registers::LCDControlBits::ObjEnable)) {
			for (int si = static_cast<int>(m_Sprites.size()) - 1; si >= 0; --si) {
				const Sprite& sprite = m_Sprites[si];
				uint8_t sprite_x = sprite.x - 8;
				uint8_t sprite_y = sprite.y - 16;
				uint8_t sprite_height = m_LCDC.GetFlag(registers::LCDControlBits::ObjSize) ? 16 : 8;

				if (m_LY >= sprite_y && m_LY < sprite_y + sprite_height && x >= sprite_x && x < sprite_x + 8) {
					int pixel_x = x - sprite_x;
					int line_y = m_LY - sprite_y;

					if (sprite.flags & SpriteFlags::YFlip) {
						line_y = sprite_height - 1 - line_y;
					}

					int tile_index = sprite.tile_index;
					if (sprite_height == 16) tile_index &= 0xfe;

					uint16_t tile_addr = static_cast<uint16_t>(0x8000 + tile_index * 16);
					if (line_y >= 8) {
						tile_addr += 16;
						line_y -= 8;
					}

					if (sprite.flags & SpriteFlags::XFlip) pixel_x = 7 - pixel_x;

					uint8_t low_byte  = ReadVRAM(tile_addr + line_y * 2);
					uint8_t high_byte = ReadVRAM(tile_addr + line_y * 2 + 1);

					int bit_index = 7 - pixel_x;
					uint8_t sprite_color_index = static_cast<uint8_t>(((high_byte >> bit_index) & 1) << 1 | ((low_byte >> bit_index) & 1));

					if (sprite_color_index != 0) {
						uint8_t color = (sprite.flags & SpriteFlags::Palette) ? m_OBP1[sprite_color_index] : m_OBP0[sprite_color_index];
						if (!((sprite.flags & SpriteFlags::Priority) && bg_window_color != 0)) {
							m_Frame[m_LY * WIDTH + x] = color;
						}
					}
				}
			}
		}
	}

	if (window_visible_this_line) {
		m_WindowLine++;
	}
}