#ifndef JOYPAD_H
#define JOYPAD_H

#include <stdint.h>
#include <print>

namespace dmg::joypad {
	enum Button : uint8_t {
		Start	= 0b10000000,
		Select	= 0b01000000,
		B		= 0b00100000,
		A		= 0b00010000,
		Down	= 0b00001000,
		Up		= 0b00000100,
		Left	= 0b00000010,
		Right	= 0b00000001,
	};

	class Joypad {
	public:
		void SetButtonState(Button button, bool pressed) {
			if (pressed) {
				m_Buttons &= ~button;
			} else {
				m_Buttons |= button;
			}
		}

		uint8_t GetP1() {
			if (!m_SelectButtons && !m_SelectDPad) {
				return m_TopNibble | 0x0f;
			}
			
			else if (m_SelectButtons && !m_SelectDPad) {
				return m_TopNibble | ((m_Buttons >> 4) & 0b1111);
			}
			
			else if (!m_SelectButtons && m_SelectDPad) {
				return m_TopNibble | ((m_Buttons >> 0) & 0b1111);
			}

			else {
				return m_TopNibble | 0x0f;
			}
		}

		void SetP1(uint8_t bits) {
			if (bits & 0b00100000) m_SelectButtons = true;
			else m_SelectButtons = false;
			
			if (bits & 0b00010000) m_SelectDPad = true;
			else m_SelectDPad = false;

			m_TopNibble = bits & 0b11110000;
		}

		uint8_t ReadP1(uint16_t) {
			return GetP1();
		}

		void WriteP1(uint16_t, uint8_t bits) { SetP1(bits); }

	private:
		uint8_t m_TopNibble = 0;
		uint8_t m_Buttons = 0xff;
		bool m_SelectButtons = false;
		bool m_SelectDPad = false;
	};
}


#endif