#include "ppu.h"
#include "bus.h"

using namespace dmg::ppu;
using namespace dmg::ppu::registers;

void PPU::Update() {
	m_STAT.SetLycIsLy(m_LY == m_LYC);

	// if LYC interrupt is enabled then request the interrupt in IF
	if (m_LY == m_LYC) {
		if (m_STAT.GetFlag(LCDStatusBits::LycIntSelect)) {
			m_Bus->RequestInterrupt(dmg::bus::InterruptFlag::LCD);
		}
	}

	// mode 0 interrupt
	if (m_STAT.GetFlag(LCDStatusBits::Mode0IntSelect) && m_Mode == 0 && m_LastMode != 0) {
		m_Bus->RequestInterrupt(dmg::bus::InterruptFlag::LCD);
	}

	// mode 1 interrupt
	if (m_STAT.GetFlag(LCDStatusBits::Mode1IntSelect) && m_Mode == 1 && m_LastMode != 1) {
		m_Bus->RequestInterrupt(dmg::bus::InterruptFlag::LCD);
	}

	// mode 2 interrupt
	if (m_STAT.GetFlag(LCDStatusBits::Mode2IntSelect) && m_Mode == 2 && m_LastMode != 2) {
		m_Bus->RequestInterrupt(dmg::bus::InterruptFlag::LCD);
	}

	// set the PPU mode
	m_STAT.SetPPUMode(m_Mode);
}

// TODO: this should take 640 t-cycles
void PPU::DMATransferOAM(uint16_t, uint8_t value) {
	for (size_t i = 0; i < 0x9f; i++) {
		m_OAM[i] = m_Bus->ReadMemory((value << 8) | i);
	}
}

void PPU::Tick() {
	if (!m_LCDC.GetLcdPpuEnabled()) {
		m_STAT.SetPPUMode(0);
		return;
	}

	m_Dots++;

	switch (m_Mode) {
		// Horizontal blank
		case 0: {
			if (m_Dots >= (376 - m_M3Duration)) {
				m_Dots = 0;
				m_LY++;
				if (m_LY >= m_WY && m_LCDC.GetWindowEnabled()) {
					m_WindowLine++;
				}

				if (m_LY == 144) {
					m_Mode = 1;

					if (!m_InVBlank) {
						m_Bus->RequestInterrupt(dmg::bus::InterruptFlag::VBlank);
						m_InVBlank = true;
						m_ShouldRender = true;
					}
				}
				
				else {
					m_Mode = 2;
				}
				break;
			}

			break;
		}

		// OAM scan
		case 2: {
			if (m_Dots == 0) {
				m_Sprites.clear();
			}

			if (m_Dots < 80 && (m_Dots % 2 == 0)) {
				size_t oam_index = (m_Dots / 2) * 4;
				if (oam_index + 3 < 0xa0) {
					uint8_t y = m_OAM[oam_index];
					uint8_t x = m_OAM[oam_index + 1];
					uint8_t tile = m_OAM[oam_index + 2];
					SpriteFlags flags = static_cast<SpriteFlags>(m_OAM[oam_index + 3]);

					int sprite_height = m_LCDC.GetObjectSize();
					uint8_t sprite_y = y;

					if (m_LY + 16 >= sprite_y && m_LY + 16 < sprite_y + sprite_height && m_Sprites.size() < 10) {
						m_Sprites.emplace_back(y, x, tile, flags);
					}
				}
			}

			if (m_Dots >= 80) {
				m_Mode = 3;
				break;
			}

			break;
		}

		// Drawing pixels
		case 3: {
			if (m_Dots >= (80 + m_M3Duration)) {
				RenderScanline();
				m_Mode = 0;
				break;
			}

			break;
		}

		// Vertical blank
		case 1: {
			if (m_Dots >= 456) {
				m_Dots = 0;
				m_LY++;

				if (m_LY > 153) {
					m_WindowLine = 0;
					m_LY = 0;
					m_Mode = 2;
					m_M3Duration = 172;
					m_InVBlank = false;
				}
			}
			break;
		}

		default: {
			std::println("ppu: invalid mode {}!", m_Mode);
			exit(1);
			break;
		}
	}

	Update();
	m_LastMode = m_Mode;
}

void PPU::RenderScanline() {
	if (m_LY >= 144) return;

	uint8_t bg_y = (m_SCY + m_LY) & 0xff;
	uint8_t bg_tile_row = bg_y / 8;
	uint8_t bg_pixel_y = bg_y % 8;

	bool window_enabled = m_LCDC.GetWindowEnabled() && (m_LY >= m_WY);
	uint8_t window_tile_row = m_WindowLine / 8;
	uint8_t window_pixel_y = m_WindowLine % 8;

	//std::println("{}: {:08b} {}", m_LY, m_LCDC.Get(), m_LCDC.GetBgWindowEnabled());
	for (int x = 0; x < WIDTH; x++) {
		uint8_t bg_color_index = 0;

		// background and window
		if (m_LCDC.GetBgWindowEnabled()) {
			bool use_window = window_enabled && (x >= (m_WX - 7));

			if (use_window) {
				uint8_t window_x = x - (m_WX - 7);
				uint8_t tile_col = window_x / 8;
				uint8_t pixel_x = window_x % 8;

				uint16_t map_address = m_LCDC.GetWindowTileMapAreaAddress() + window_tile_row * 32 + tile_col;
				uint8_t tile_index = ReadVRAM(map_address);

				uint16_t tile_data_base = m_LCDC.GetBgWindowTileDataAreaAddress();
				int tile_number = (tile_data_base == 0x8800) ? static_cast<int8_t>(tile_index) : tile_index;
				uint16_t tile_address = tile_data_base + tile_number * 16;

				uint8_t low_byte = ReadVRAM(tile_address + window_pixel_y * 2);
				uint8_t high_byte = ReadVRAM(tile_address + window_pixel_y * 2 + 1);

				uint8_t bit_index = 7 - pixel_x;
				bg_color_index = ((high_byte >> bit_index) & 1) << 1 | ((low_byte >> bit_index) & 1);
			}
			
			else {
				uint8_t bg_x = (m_SCX + x) & 0xff;
				uint8_t tile_col = bg_x / 8;
				uint8_t pixel_x = bg_x % 8;

				uint16_t map_address = m_LCDC.GetBgTileMapAreaAddress() + bg_tile_row * 32 + tile_col;
				uint8_t tile_index = ReadVRAM(map_address);

				uint16_t tile_data_base = m_LCDC.GetBgWindowTileDataAreaAddress();
				int tile_number = (tile_data_base == 0x8800) ? static_cast<int8_t>(tile_index) : tile_index;
				uint16_t tile_address = tile_data_base + tile_number * 16;

				uint8_t low_byte = ReadVRAM(tile_address + bg_pixel_y * 2);
				uint8_t high_byte = ReadVRAM(tile_address + bg_pixel_y * 2 + 1);

				uint8_t bit_index = 7 - pixel_x;
				bg_color_index = ((high_byte >> bit_index) & 1) << 1 | ((low_byte >> bit_index) & 1);
			}
		}

		m_Frame[m_LY * WIDTH + x] = m_BGP[bg_color_index];

		// sprites
		if (m_LCDC.GetObjectEnabled()) {
			for (const Sprite& sprite : m_Sprites) {
				int sprite_x = sprite.x - 8;
				int sprite_y = sprite.y - 16;
				int sprite_height = m_LCDC.GetObjectSize();

				if (m_LY >= sprite_y && m_LY < sprite_y + sprite_height && x >= sprite_x && x < sprite_x + 8) {
					uint8_t pixel_x = x - sprite_x;
					uint8_t pixel_y = m_LY - sprite_y;

					if (sprite.flags & SpriteFlags::YFlip) pixel_y = (sprite_height - 1) - pixel_y;
					if (sprite.flags & SpriteFlags::XFlip) pixel_x = 7 - pixel_x;

					uint16_t tile_index = sprite.tile;
					if (sprite_height == 16) {
						tile_index = (tile_index & 0xfe) + (pixel_y >= 8 ? 1 : 0);
						pixel_y %= 8;
					}

					uint16_t tile_address = 0x8000 + tile_index * 16;
					uint8_t low_byte = ReadVRAM(tile_address + pixel_y * 2);
					uint8_t high_byte = ReadVRAM(tile_address + pixel_y * 2 + 1);

					uint8_t bit_index = 7 - pixel_x;
					uint8_t sprite_color_index = ((high_byte >> bit_index) & 1) << 1 | ((low_byte >> bit_index) & 1);

					if (sprite_color_index == 0) continue;
					if (sprite.flags & SpriteFlags::ObjToBgPriority && bg_color_index != 0) continue;

					m_Frame[m_LY * WIDTH + x] = (sprite.flags & SpriteFlags::OBP1) ? m_OBP1[sprite_color_index] : m_OBP0[sprite_color_index];
				}
			}
		}
	}
}