#ifndef freq_h
#define freq_h

#include "main.h"

static constexpr char MQTT_FREQ_TOPIC[]   = "freq";


class freq : public peripheral {
	public:
		freq();
		~freq();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
		uint8_t* get_key();
	private:
		uint16_t m_update;
		uint32_t m_last_update;
		bool m_pin_state;
		uint8_t key[3];
		uint8_t m_pin;
		mqtt_parameter_16 m_counter;
};


#endif
