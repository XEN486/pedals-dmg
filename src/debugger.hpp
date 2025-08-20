#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include "cpu/cpu.hpp"
#include "peripherals/bus.hpp"
#include "cartridge/cartridge.hpp"
#include "peripherals/joypad.hpp"
#include "peripherals/timer.hpp"
#include "ppu/ppu.hpp"

#include "thirdparty/imgui.h"
#include "thirdparty/imgui_memory_editor.h"

// TODO: make the palette be an std::array<uint32_t, 5>& or something instead of a uint32_t*

namespace pedals::debugger {
	class DebugUI {
	public:
		DebugUI(std::shared_ptr<pedals::cpu::SM83> cpu, std::shared_ptr<pedals::bus::Bus> bus, std::shared_ptr<pedals::timer::Timer> timer, std::shared_ptr<pedals::ppu::PPU> ppu, std::shared_ptr<pedals::cartridge::Cartridge> cartridge, uint32_t* palette)
			: m_CPU(cpu), m_Bus(bus), m_Timer(timer), m_PPU(ppu), m_Cartridge(cartridge), m_Palette(palette) {
				m_MemoryEditor.OptShowAscii = false;
				m_MemoryEditor.OptUpperCaseHex = false;
			}

		void Draw() {
			// CPU window
			ImGui::Begin("SM83");
				ImGui::SeparatorText("Controls");
				CPU_DrawControlButtons();

				ImGui::SeparatorText("Stack");
				CPU_DrawStackControls();

				ImGui::SeparatorText("Registers");
				CPU_DrawRegisters();

				ImGui::SeparatorText("Interrupts");
				CPU_DrawInterrupts();

				ImGui::SeparatorText("Disassembly");
				CPU_DrawDisassembly();
			ImGui::End();

			// bus windows
			BUS_DrawHexEditors();

			// PPU windows
			PPU_DrawPalette();
			PPU_DrawOAM();
		}

	public:
		bool& GetSingleStep() {
			return m_SingleStep;
		}

		bool& GetBreakOnInterrupt() {
			return m_BreakOnInterrupt;
		}

		bool& GetBreakOnRETI() {
			return m_BreakOnRETI;
		}

	private:
		void CPU_DrawControlButtons();
		void CPU_DrawStackControls();
		void CPU_DrawRegisters();
		void CPU_DrawInterrupts();
		void CPU_DrawDisassembly();
		void BUS_DrawHexEditors();
		void PPU_DrawPalette();
		void PPU_DrawOAM();

	private:
		std::shared_ptr<pedals::cpu::SM83> m_CPU;
		std::shared_ptr<pedals::bus::Bus> m_Bus;
		std::shared_ptr<pedals::timer::Timer> m_Timer;
		std::shared_ptr<pedals::ppu::PPU> m_PPU;
		std::shared_ptr<pedals::cartridge::Cartridge> m_Cartridge;

		bool m_SingleStep = false;
		bool m_BreakOnInterrupt = false;
		bool m_BreakOnRETI = false;

		MemoryEditor m_MemoryEditor {};
		uint16_t m_PushValue = 0;
		uint32_t* m_Palette;
	};
}

#endif