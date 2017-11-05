#ifndef dimmer_h
#define dimmer_h

#include <Arduino.h>
#include "mqtt_parameter.h"
#include "main.h"

class dimmer {
	public:
		dimmer();
		~dimmer();
		bool init();
		bool loop();
		bool intervall_update();
		bool subscribe();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		uint8_t get_key();
	private:
		bool publish();
		mqtt_parameter_8 m_state;

};


#endif
