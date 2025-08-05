#include "timer.h"
#include "bus.h"
using namespace pedals::timer;

void Timer::Tick() {
	m_Cycles++;

	if ((m_Cycles % 256) == 0) {
		m_DIV++;
	}

	if (m_TIMAenabled) {
		uint32_t cycles_per_increment = 0;
		switch (m_TAC & 0b11) {
			case 0b00: cycles_per_increment = 1024; break;
			case 0b01: cycles_per_increment = 16; break;
			case 0b10: cycles_per_increment = 64; break;
			case 0b11: cycles_per_increment = 256; break;
		}

		if ((m_Cycles % cycles_per_increment) == 0) {
			m_TIMA++;

			if (m_TIMA == 0x00) {
				m_TIMA = m_TMA;
				m_Bus->RequestInterrupt(pedals::bus::InterruptFlag::Timer);
			}
		}
	}
}

void Timer::SetTAC(uint8_t bits) {
	//std::println("timer: tac = {:08b}", bits);

	bool was_enabled = m_TIMAenabled;
	uint8_t old_speed = m_TAC & 0b11;

	m_TAC = bits;
	m_TIMAenabled = bits & 0b00000100;

	if (was_enabled && m_TIMAenabled) {
		uint8_t new_speed = bits & 0b11;
		if ((old_speed == 0b11 && new_speed == 0b00) ||
			(old_speed == 0b00 && new_speed == 0b11)) {
				m_TIMA++;

				if (m_TIMA == 0x00) {
					m_TIMA = m_TMA;
					m_Bus->RequestInterrupt(pedals::bus::InterruptFlag::Timer);
				}
			}
	}
}

uint8_t Timer::GetTAC() {
	return m_TAC;
}