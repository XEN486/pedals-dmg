#include "cpu/cpu.h"
#include "peripherals/bus.h"
#include "cartridge/cartridge.h"
#include "ppu/ppu.h"
#include "peripherals/joypad.h"
#include "peripherals/timer.h"

#include <print>
#include <fstream>
#include <cstdio>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlrenderer3.h"

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

static void cpu_ui(std::shared_ptr<pedals::cpu::SM83> cpu, std::shared_ptr<pedals::bus::Bus> bus, std::shared_ptr<pedals::timer::Timer> timer, std::shared_ptr<pedals::ppu::PPU> ppu, bool& single_step) {
	auto& regs = cpu->GetRegisters();
	std::string disassembly = pedals::debugger::DisassembleInstruction(bus, regs.pc);
	static uint16_t stack_push = 0;

	// CPU state viewer/editor
	ImGui::Begin("SM83");
		ImGui::Text("Controls");

		// CPU single step mode
		if (ImGui::Button(single_step ? "Unpause" : "Pause")) {
			single_step = !single_step;
		}

		ImGui::SameLine();

		// CPU step
		if (ImGui::Button("Step")) {
			uint8_t step_cycles = cpu->Step();
			for (size_t i = 0; i < step_cycles; i++) {
				ppu->Tick();
				timer->Tick();
			}
		}

		ImGui::SameLine();

		// CPU reset
		if (ImGui::Button("Reset")) {
			cpu->Reset();
			bus->SetBootROMVisibility(true);
		}

		// byte to push to stack
		ImGui::PushItemWidth(36);
			ImGui::InputScalar("##stackpush", ImGuiDataType_U16, &stack_push, nullptr, nullptr, "%04x", ImGuiInputTextFlags_None);
		ImGui::PopItemWidth();

		ImGui::SameLine();

		// push byte to stack
		if (ImGui::Button("Push Byte")) {
			cpu->StackPush8(stack_push);
		}

		ImGui::SameLine();

		// push word to stack
		if (ImGui::Button("Push Word")) {
			cpu->StackPush16(stack_push);
		}

		ImGui::Text("Registers");
		ImGui::PushItemWidth(36);
			// pair register row
			ImGui::InputScalar("AF", ImGuiDataType_U16, &regs.af, nullptr, nullptr, "%04x", ImGuiInputTextFlags_None);
			ImGui::SameLine();
			ImGui::InputScalar("BC", ImGuiDataType_U16, &regs.bc, nullptr, nullptr, "%04x", ImGuiInputTextFlags_None);
			ImGui::SameLine();
			ImGui::InputScalar("DE", ImGuiDataType_U16, &regs.de, nullptr, nullptr, "%04x", ImGuiInputTextFlags_None);
			ImGui::SameLine();
			ImGui::InputScalar("HL", ImGuiDataType_U16, &regs.hl, nullptr, nullptr, "%04x", ImGuiInputTextFlags_None);
			
			// pc and sp row
			ImGui::InputScalar("PC", ImGuiDataType_U16, &regs.pc, nullptr, nullptr, "%04x", ImGuiInputTextFlags_None);
			ImGui::SameLine();
			ImGui::InputScalar("SP", ImGuiDataType_U16, &regs.sp, nullptr, nullptr, "%04x", ImGuiInputTextFlags_None);
		ImGui::PopItemWidth();

		ImGui::SameLine();

		// instruction at PC
		ImGui::Text("%s", disassembly.c_str());

	ImGui::End();
}

int main(int argc, char** argv) {
	// check if we have enough arguments
	if (argc < 3) {	
		std::println("usage: {} [boot rom] [.gb rom]", argv[0]);
		return 1;
	}

	// initialize SDL
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::println(stderr, "SDL_Init(): {}", SDL_GetError());
	}

	// initialize SDL stuff
	SDL_Window* window = SDL_CreateWindow("Pedals DMG", 1600, 900, SDL_WINDOW_RESIZABLE);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	// create imgui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// enable docking features
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// dark colour scheme
	ImGui::StyleColorsDark();

	// initialize imgui
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	// initialize the palette
	init_palette(SDL_PIXELFORMAT_RGBA8888);

	// initialize the main components and peripherals
	auto ppu	= std::make_shared<pedals::ppu::PPU>();
	auto timer	= std::make_shared<pedals::timer::Timer>();
	auto joypad	= std::make_shared<pedals::joypad::Joypad>();
	auto cart	= std::make_shared<pedals::cartridge::Cartridge>(argv[2]);
	auto bus	= std::make_shared<pedals::bus::Bus>(ppu, joypad, timer, cart);
	auto cpu	= std::make_shared<pedals::cpu::SM83>(bus);

	// we set the bus here to stop circular dependencies
	ppu->SetBus(bus);
	timer->SetBus(bus);

	// load boot ROM
	bus->LoadBootROM(argv[1]);
	cpu->Reset();

	// event and emulator state
	SDL_Event event;
	bool running = true;
	bool single_step = false;

	// timing constants
	const int cycles_per_frame = 70224;
	const double frame_time_ms = 1000.0 / 59.7275;
	uint64_t last_frame = SDL_GetPerformanceCounter();

	// main loop
	while (running) {
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);

			switch (event.type) {
				case SDL_EVENT_QUIT:
					running = false;
					break;

				case SDL_EVENT_KEY_DOWN:
					// screenshot
					if (event.key.key == SDLK_F12) {
						SDL_Surface* temp_surface = SDL_CreateSurfaceFrom(WIDTH, HEIGHT, SDL_PIXELFORMAT_RGBA8888, frame, WIDTH * sizeof(uint32_t));
						IMG_SavePNG(temp_surface, "screenshot.png");
						SDL_DestroySurface(temp_surface);
					}

					// joypad
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
					// joypad
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

		// run a frame worth of emulation
		uint32_t frame_cycles = 0;
		while (frame_cycles < cycles_per_frame && !single_step) {
			uint8_t step_cycles = cpu->Step();
			frame_cycles += step_cycles;

			for (size_t i = 0; i < step_cycles; i++) {
				ppu->Tick();
				timer->Tick();
			}
		}

		// render the window if the PPU says we should render
		// or if we are single stepping to stop the window from timing out
		if (ppu->ShouldRender() || single_step) {
			// begin imgui frame
			ImGui_ImplSDLRenderer3_NewFrame();
			ImGui_ImplSDL3_NewFrame();
			ImGui::NewFrame();
			ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

			// convert the ppu indexed image to an RGBA array
			for (size_t i = 0; i < WIDTH * HEIGHT; i++) {
				frame[i] = palette[ppu->GetFrame()[i]];
			}

			// update the SDL texture
			SDL_UpdateTexture(texture, nullptr, frame, WIDTH * sizeof(uint32_t));

			// debug windows
			cpu_ui(cpu, bus, timer, ppu, single_step);

			// LCD viewport
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
				ImGui::Begin("LCD");
					ImVec2 content_size = ImGui::GetContentRegionAvail();

					float window_aspect = content_size.x / content_size.y;
					float image_aspect = float(WIDTH) / float(HEIGHT);

					float scale;
					if (window_aspect > image_aspect) {
						scale = content_size.y / float(HEIGHT);
					} else {
						scale = content_size.x / float(WIDTH);
					}

					ImVec2 image_size = ImVec2(WIDTH * scale, HEIGHT * scale);
					ImVec2 cursor_pos = ImGui::GetCursorPos();

					float offset_x = (content_size.x - image_size.x) * 0.5f;
					float offset_y = (content_size.y - image_size.y) * 0.5f;

					ImGui::SetCursorPos(ImVec2(cursor_pos.x + offset_x, cursor_pos.y + offset_y));
					ImGui::Image((void*)texture, image_size);
				ImGui::End();
			ImGui::PopStyleVar();

			// prepare the drawlist
			ImGui::Render();
			SDL_RenderClear(renderer);

			// render and present the drawlist
			ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
			SDL_RenderPresent(renderer);
		}

		// delay for next frame
		uint64_t now = SDL_GetPerformanceCounter();
		uint64_t elapsed = (now - last_frame) * 1000 / SDL_GetPerformanceFrequency();
		if (elapsed < frame_time_ms) {
			SDL_Delay(static_cast<uint32_t>(frame_time_ms - elapsed));
		}

		// set delta time
		last_frame = SDL_GetPerformanceCounter();
	}

	// cleanup imgui stuff
	ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

	// cleanup SDL stuff
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}