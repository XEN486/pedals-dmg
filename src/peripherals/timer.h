#ifndef TIMER_H
#define TIMER_H

#include <memory>
#include <stdint.h>

namespace pedals::bus {
	class Bus;
}

namespace pedals::timer {
	class Timer {
	public:
		void SetBus(std::shared_ptr<pedals::bus::Bus> bus) {
			m_Bus = bus;
		}

		void Tick();

		void SetTAC(uint8_t bits);
		uint8_t GetTAC();

		uint8_t ReadDIV(uint16_t _) {
			return m_DIV;
		}

		uint8_t ReadTIMA(uint16_t _) {
			return m_TIMA;
		}

		uint8_t ReadTMA(uint16_t _) {
			return m_TMA;
		}

		uint8_t ReadTAC(uint16_t _) {
			return GetTAC();
		}

		void WriteDIV(uint16_t, uint8_t) {
			m_DIV = 0;
			m_Cycles = 0;
		}

		void WriteTIMA(uint16_t, uint8_t value) {
			m_TIMA = value;
		}

		void WriteTMA(uint16_t, uint8_t value) {
			m_TMA = value;
		}

		void WriteTAC(uint16_t, uint8_t bits) {
			SetTAC(bits);
		}
	
	private:
		uint8_t m_DIV = 0;
		uint8_t m_TIMA = 0;
		uint8_t m_TMA = 0;
		uint8_t m_TAC = 0;
		
		bool m_TIMAenabled = false;

		size_t m_Cycles = 0;

		std::shared_ptr<pedals::bus::Bus> m_Bus;
	};
}

#endif