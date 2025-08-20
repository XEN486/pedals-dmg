#include "debugger.hpp"
using namespace pedals::debugger;

#define RGBA(x) ((x & 0xff000000) >> 24) / 255.0f, ((x & 0x00ff0000) >> 16) / 255.0f, ((x & 0x0000ff00) >> 8) / 255.0f, ((x & 0x000000ff) >> 0) / 255.0f

void DebugUI::CPU_DrawControlButtons() {
    if (ImGui::Button(m_SingleStep ? "Unpause" : "Pause")) {
        m_SingleStep = !m_SingleStep;
    }

    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        uint8_t step_cycles = m_CPU->Step();
        for (size_t i = 0; i < step_cycles; i++) {
            m_PPU->Tick();
            m_Timer->Tick();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        m_CPU->Reset();
        m_Bus->SetBootROMVisibility(true);
    }
}

void DebugUI::CPU_DrawStackControls() {
    ImGui::PushItemWidth(36);
    ImGui::InputScalar("##stackpush", ImGuiDataType_U16, &m_PushValue, nullptr, nullptr, "%04x");
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Push Byte")) m_CPU->StackPush8(static_cast<uint8_t>(m_PushValue));
    ImGui::SameLine();
    if (ImGui::Button("Push Word")) m_CPU->StackPush16(m_PushValue);
}

void DebugUI::CPU_DrawRegisters() {
	auto& regs = m_CPU->GetRegistersRef();
    ImGui::PushItemWidth(36);

    ImGui::InputScalar("AF", ImGuiDataType_U16, &regs.af, nullptr, nullptr, "%04x"); ImGui::SameLine();
    ImGui::InputScalar("BC", ImGuiDataType_U16, &regs.bc, nullptr, nullptr, "%04x"); ImGui::SameLine();
    ImGui::InputScalar("DE", ImGuiDataType_U16, &regs.de, nullptr, nullptr, "%04x"); ImGui::SameLine();
    ImGui::InputScalar("HL", ImGuiDataType_U16, &regs.hl, nullptr, nullptr, "%04x");

    ImGui::InputScalar("PC", ImGuiDataType_U16, &regs.pc, nullptr, nullptr, "%04x"); ImGui::SameLine();
    ImGui::InputScalar("SP", ImGuiDataType_U16, &regs.sp, nullptr, nullptr, "%04x");

    ImGui::PopItemWidth();
}

void DebugUI::CPU_DrawInterrupts() {
	ImGui::Text("%s interrupt", m_CPU->InInterrupt() ? "Handling an" : "Not in an");
    ImGui::PushItemWidth(16);
    ImGui::InputScalar("IME", ImGuiDataType_U8, &m_CPU->GetIMERef(), nullptr, nullptr, "%d");
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(64);
    ImGui::SameLine();
    ImGui::InputScalar("IE", ImGuiDataType_U8, &m_Bus->GetIERef(), nullptr, nullptr, "%08b");
    ImGui::SameLine();
    ImGui::InputScalar("IF", ImGuiDataType_U8, &m_Bus->GetIFRef(), nullptr, nullptr, "%08b");
    ImGui::PopItemWidth();

    if (ImGui::Button(m_BreakOnInterrupt ? "Enabled" : "Break on Interrupt")) {
        m_BreakOnInterrupt = true;
        m_BreakOnRETI = false;
    }

    ImGui::SameLine();
    if (ImGui::Button(m_BreakOnRETI ? "Enabled" : "Break on RETI")) {
        m_BreakOnRETI = true;
        m_BreakOnInterrupt = false;
    }
}

void DebugUI::CPU_DrawDisassembly() {
	uint16_t pc = m_CPU->GetRegistersRef().pc;
    std::string disasm = pedals::cpu::DisassembleInstruction(m_Bus, pc);
    ImGui::Text("%04x: %s", pc, disasm.c_str());
}

void DebugUI::BUS_DrawHexEditors() {
	m_MemoryEditor.DrawWindow("Cartridge", m_Cartridge->GetRawRef().data(), m_Cartridge->GetRawRef().size());
	m_MemoryEditor.DrawWindow("Boot ROM", m_Bus->GetBootROMRef().data(), m_Bus->GetBootROMRef().size());
	m_MemoryEditor.DrawWindow("Work RAM", m_Bus->GetWorkRAMRef().data(), m_Bus->GetWorkRAMRef().size());
	m_MemoryEditor.DrawWindow("High RAM", m_Bus->GetHighRAMRef().data(), m_Bus->GetHighRAMRef().size());
}

static void draw_palette_button(uint32_t* palette, std::string_view label, uint8_t& color_index) {
	ImGui::ColorButton(label.data(), ImVec4(RGBA(palette[color_index])), ImGuiColorEditFlags_NoPicker, ImVec2(48, 48));

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PALETTE_COLOR_INDEX")) {
			if (payload->DataSize == sizeof(int)) {
				int new_index = *(const int*)payload->Data;
				color_index = static_cast<uint8_t>(new_index);
			}
		}
		ImGui::EndDragDropTarget();
	}
}

void DebugUI::PPU_DrawPalette() {
	ImGui::Begin("Palette");

	ImGui::Text("BGP");
	for (int i = 0; i < 4; ++i) {
		draw_palette_button(m_Palette, "BGP[" + std::to_string(i) + "]", m_PPU->GetBGPRef()[i]);
		ImGui::SameLine();
	}

	ImGui::Dummy(ImVec2(48, 0));
	ImGui::SameLine();

	for (int i = 0; i < 2; ++i) {
		std::string label = "Color " + std::to_string(i);
		ImGui::ColorButton(label.c_str(), ImVec4(RGBA(m_Palette[i])), ImGuiColorEditFlags_NoPicker, ImVec2(48, 48));

		if (ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload("PALETTE_COLOR_INDEX", &i, sizeof(int));
			ImGui::Text("Drag Color %d", i);
			ImGui::EndDragDropSource();
		}

		if (i % 2 == 0) ImGui::SameLine();
	}

	ImGui::Text("OBP0");
	
	for (int i = 0; i < 4; ++i) {
		draw_palette_button(m_Palette, "OBP0[" + std::to_string(i) + "]", m_PPU->GetOBP0Ref()[i]);
		ImGui::SameLine();
	}

	ImGui::Dummy(ImVec2(48, 0));
	ImGui::SameLine();
	
	for (int i = 2; i < 4; ++i) {
		std::string label = "Color " + std::to_string(i);
		ImGui::ColorButton(label.c_str(), ImVec4(RGBA(m_Palette[i])), ImGuiColorEditFlags_NoPicker, ImVec2(48, 48));

		if (ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload("PALETTE_COLOR_INDEX", &i, sizeof(int));
			ImGui::Text("Drag Color %d", i);
			ImGui::EndDragDropSource();
		}

		if (i % 2 == 0) ImGui::SameLine();
	}

	ImGui::Text("OBP1");
	for (int i = 0; i < 4; ++i) {
		draw_palette_button(m_Palette, "OBP1[" + std::to_string(i) + "]", m_PPU->GetOBP1Ref()[i]);
		if (i < 3) ImGui::SameLine();
	}

	ImGui::End();
}

void DebugUI::PPU_DrawOAM() {
	ImGui::Begin("OAM");
	ImGui::End();
}