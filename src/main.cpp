#include "cpu/cpu.h"
#include "peripherals/bus.h"
#include "cartridge/cartridge.h"
#include "ppu/ppu.h"
#include "peripherals/joypad.h"
#include "peripherals/timer.h"

#include <print>
#include <fstream>
#include <cstdio>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

static uint32_t frame[WIDTH * HEIGHT];
static uint32_t palette[5];

static void init_palette(SDL_PixelFormat pfmt) {
	const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(pfmt);
	palette[0] = SDL_MapRGBA(fmt, nullptr, 0xc6, 0xde, 0x8c, 255);
	palette[1] = SDL_MapRGBA(fmt, nullptr, 0x84, 0xa5, 0x63, 255);
	palette[2] = SDL_MapRGBA(fmt, nullptr, 0x39, 0x61, 0x39, 255);
	palette[3] = SDL_MapRGBA(fmt, nullptr, 0x08, 0x18, 0x10, 255);
	palette[4] = SDL_MapRGBA(fmt, nullptr, 0xd2, 0xe6, 0xa6, 255); // LCD off color
}

int main(int argc, char** argv) {
	if (argc < 3) {	
		std::println("usage: {} [boot rom] [.gb rom]", argv[0]);
		return 1;
	}

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Pedals DMG", WIDTH * 2, HEIGHT * 2, SDL_WINDOW_RESIZABLE);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
	SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

	init_palette(SDL_PIXELFORMAT_ARGB8888);

	auto ppu = std::make_shared<pedals::ppu::PPU>(pedals::ppu::PPU());
	auto timer = std::make_shared<pedals::timer::Timer>(pedals::timer::Timer());
	auto joypad = std::make_shared<pedals::joypad::Joypad>(pedals::joypad::Joypad());
	auto cart = std::make_shared<pedals::cartridge::Cartridge>(argv[2]);
	auto bus = std::make_shared<pedals::bus::Bus>(pedals::bus::Bus(ppu, joypad, timer, cart));
	auto cpu = pedals::cpu::SM83(bus);

	ppu->SetBus(bus);
	timer->SetBus(bus);

	// load boot ROM
	bus->LoadBootROM(argv[1]);
	cpu.Reset();

	SDL_Event event;
	bool running = true;
	bool single_step = false;

	const int cycles_per_frame = 70224;
	const double frame_time_ms = 1000.0 / 59.7275;
	uint64_t last_frame = SDL_GetPerformanceCounter();

	while (running) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
					running = false;
					break;

				case SDL_EVENT_KEY_DOWN:
					if (event.key.key == SDLK_F12) {
						SDL_Surface* temp_surface = SDL_CreateSurfaceFrom(WIDTH, HEIGHT, SDL_PIXELFORMAT_ARGB8888, frame, WIDTH * sizeof(uint32_t));
						IMG_SavePNG(temp_surface, "screenshot.png");
						SDL_DestroySurface(temp_surface);
					}

					// F10 - dump CPU state
					if (event.key.key == SDLK_F10) {
						cpu.Dump(stdout);
					}

					// F9 - toggle CPU tracing
					if (event.key.key == SDLK_F9) {
						cpu.ToggleDump();
					}

					// F8 - toggle single step mode
					if (event.key.key == SDLK_F8) {
						single_step = !single_step;
						std::println("single step mode {}", single_step ? "enabled" : "disabled");
					}

					// F7 - step CPU and dump state
					if (event.key.key == SDLK_F7) {
						if (!single_step) break;
						cpu.Step();
						cpu.Dump(stdout);
					}

					if (event.key.key == SDLK_S) joypad->SetButtonState(pedals::joypad::Button::B, true);
					if (event.key.key == SDLK_A) joypad->SetButtonState(pedals::joypad::Button::A, true);
					if (event.key.key == SDLK_RETURN) joypad->SetButtonState(pedals::joypad::Button::Start, true);
					if (event.key.key == SDLK_SPACE) joypad->SetButtonState(pedals::joypad::Button::Select, true);
					if (event.key.key == SDLK_UP) joypad->SetButtonState(pedals::joypad::Button::Up, true);
					if (event.key.key == SDLK_DOWN) joypad->SetButtonState(pedals::joypad::Button::Down, true);
					if (event.key.key == SDLK_LEFT) joypad->SetButtonState(pedals::joypad::Button::Left, true);
					if (event.key.key == SDLK_RIGHT) joypad->SetButtonState(pedals::joypad::Button::Right, true);
					break;

				case SDL_EVENT_KEY_UP:
					if (event.key.key == SDLK_S) joypad->SetButtonState(pedals::joypad::Button::B, false);
					if (event.key.key == SDLK_A) joypad->SetButtonState(pedals::joypad::Button::A, false);
					if (event.key.key == SDLK_RETURN) joypad->SetButtonState(pedals::joypad::Button::Start, false);
					if (event.key.key == SDLK_SPACE) joypad->SetButtonState(pedals::joypad::Button::Select, false);
					if (event.key.key == SDLK_UP) joypad->SetButtonState(pedals::joypad::Button::Up, false);
					if (event.key.key == SDLK_DOWN) joypad->SetButtonState(pedals::joypad::Button::Down, false);
					if (event.key.key == SDLK_LEFT) joypad->SetButtonState(pedals::joypad::Button::Left, false);
					if (event.key.key == SDLK_RIGHT) joypad->SetButtonState(pedals::joypad::Button::Right, false);
					break;
			}
		}

		uint32_t frame_cycles = 0;

		while (frame_cycles < cycles_per_frame) {
			uint8_t step_cycles = cpu.Step();
			frame_cycles += step_cycles;

			/*
			if (cpu.GetRegisters().pc == 0x100) {
				cpu.ToggleDump();
			}
			*/

			for (size_t i = 0; i < step_cycles; i++) {
				ppu->Tick();
				timer->Tick();
			}
		}

		if (ppu->ShouldRender()) {
			for (size_t i = 0; i < WIDTH * HEIGHT; i++) {
				frame[i] = palette[ppu->GetFrame()[i]];
			}

			SDL_UpdateTexture(texture, nullptr, frame, WIDTH * sizeof(uint32_t));
			SDL_RenderClear(renderer);
			SDL_RenderTexture(renderer, texture, nullptr, nullptr);
			SDL_RenderPresent(renderer);
		}

		uint64_t now = SDL_GetPerformanceCounter();
		uint64_t elapsed = (now - last_frame) * 1000 / SDL_GetPerformanceFrequency();

		if (elapsed < frame_time_ms) {
			SDL_Delay(static_cast<uint32_t>(frame_time_ms - elapsed));
		}

		last_frame = SDL_GetPerformanceCounter();
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}