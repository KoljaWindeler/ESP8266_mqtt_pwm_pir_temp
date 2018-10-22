#ifndef freq_h
#define freq_h

#include "main.h"

#define FREQ_KEEPOUT_MS 40 // each pulse is 86 msec
static constexpr char MQTT_FREQ_TOPIC[]   = "freq";


class freq : public capability {
	public:
		freq();
		~freq();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		uint8_t* get_key();
		bool parse(uint8_t* config);
		uint8_t count_intervall_update();
		// bool receive(uint8_t* p_topic, uint8_t* p_payload);
		// bool subscribe();
		// bool publish();
	private:
		uint16_t m_unit;
		uint32_t m_first_toggle;
		uint32_t m_last_toggle; // debounce
		bool m_pin_state;
		uint8_t m_pin;
		uint16_t m_counter;
};


#endif
