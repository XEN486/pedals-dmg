#include "sm83.h"
#include "bus.h"
#include "cartridge.h"
#include "ppu.h"

#include <print>
#include <fstream>

#include <SDL3/SDL.h>

#define WIDTH 160
#define HEIGHT 144
#define SCALE 6

static uint32_t frame[WIDTH * HEIGHT];
static uint32_t palette[4];
SDL_Renderer* renderer;
SDL_Texture* texture;

static void init_palette(SDL_PixelFormat pfmt) {
	const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(pfmt);
	palette[0] = SDL_MapRGBA(fmt, nullptr, 255, 255, 255, 255);
	palette[1] = SDL_MapRGBA(fmt, nullptr, 170, 170, 170, 255);
	palette[2] = SDL_MapRGBA(fmt, nullptr, 85, 85, 85, 255);
	palette[3] = SDL_MapRGBA(fmt, nullptr, 0, 0, 0, 255);
}

void render_tile(std::shared_ptr<gb::ppu::PPU> ppu, size_t tile_base, size_t pixel_x, size_t pixel_y) {
	for (size_t y = 0; y < 8; y++) {
		uint8_t low_byte = ppu->ReadVRAM(tile_base + y * 2);
		uint8_t high_byte = ppu->ReadVRAM(tile_base + y * 2 + 1);

		for (size_t x = 0; x < 8; x++) {
			uint8_t bit_index = 7 - x;
			uint8_t bit1 = (low_byte >> bit_index) & 1;
			uint8_t bit2 = (high_byte >> bit_index) & 1;
			uint8_t color_index = (bit2 << 1) | bit1;

			frame[(pixel_y + y) * WIDTH + (pixel_x + x)] = palette[color_index];
		}
	}
}

void render_background(std::shared_ptr<gb::ppu::PPU> ppu) {
	auto& lcdc = ppu->GetLCDControlRegister();
	
	for (size_t tile_y = 0; tile_y < 18; tile_y++) {
		for (size_t tile_x = 0; tile_x < 20; tile_x++) {
			uint16_t map_address = lcdc.GetBgTileMapAreaAddress() + tile_y * 32 + tile_x;
			uint8_t tile_index = ppu->ReadVRAM(map_address);

			uint16_t tile_address;
			if (lcdc.GetBgWindowTileDataAreaAddress() == 0x8000) {
				tile_address = 0x8000 + tile_index * 16;
			} else {
				tile_address = 0x9000 + static_cast<int8_t>(tile_index) * 16;
			}

			render_tile(ppu, tile_address, tile_x * 8, tile_y * 8);
		}
	}
}

int main(int argc, char** argv) {
	if (argc < 3) {
		std::println("usage: {} [boot rom] [.gb rom]", argv[0]);
		return 1;
	}

	std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> raw(size);
	file.read(reinterpret_cast<char*>(raw.data()), size);

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("laz pedals", WIDTH * SCALE, HEIGHT * SCALE, SDL_WINDOW_RESIZABLE);

	renderer = SDL_CreateRenderer(window, nullptr);
	SDL_SetRenderScale(renderer, SCALE, SCALE);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

	SDL_Surface* temp_surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_ARGB8888);
	init_palette(temp_surface->format);
	SDL_DestroySurface(temp_surface);

	auto ppu = std::make_shared<gb::ppu::PPU>(gb::ppu::PPU());
	auto bus = std::make_shared<gb::bus::Bus>(gb::bus::Bus(ppu));
	auto cart = gb::cartridge::Cartridge(bus, argv[2]);
	auto cpu = gb::cpu::SM83(bus);

	uint8_t mapper = bus->ReadMemory(0x147);
	if (mapper != 0) {
		std::println("mapper {} unsupported", mapper);
		return 1;
	}

	// set PPU bus
	ppu->SetBus(bus);

	// load boot ROM
	bus->LoadBootROM(raw);
	cpu.Reset();
	
	//cpu.SetDMGBootROMState();

	SDL_Event event;
	bool running = true;
	while (running) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
					running = false;
					break;
			}
		}

		uint8_t cycles = cpu.Step();
		ppu->Tick(cycles);

		if (ppu->StartedVBlank()) {
			render_background(ppu);

			SDL_UpdateTexture(texture, nullptr, frame, WIDTH * sizeof(uint32_t));
			SDL_RenderClear(renderer);
			SDL_RenderTexture(renderer, texture, nullptr, nullptr);
			SDL_RenderPresent(renderer);
		}
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}