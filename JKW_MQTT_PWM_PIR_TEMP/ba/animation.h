#ifndef animation_h
#define animation_h

#include <Arduino.h>
#include "mqtt_parameter.h"
#include "main.h"

class animation : public Interface {
	public:
		animation();
		~animation();
		bool init();
		bool loop();
		bool intervall_update();
		bool subscribe();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		uint8_t get_key();
	private:
		bool publish();
		mqtt_parameter_8 m_state;
		RgbColor wheel(byte WheelPos);
		uint8_t m_animation_pos;     // pointer im wheel
};


#endif
