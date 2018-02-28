#ifndef capability_h
#define capability_h

#include <Arduino.h>

class capability_unit {
	public:
		capability_unit();
		bool active;
		unsigned char key;
};

class capability {
	public:
		capability();
		void reg(capability_unit* u);
		bool parse_eeprom(uint8_t* e);
		bool parse(unsigned char* input);
		capability_unit m_use_neo_as_rgb;
		capability_unit m_use_my92x1_as_rgb;
		capability_unit m_use_pwm_as_rgb;
		capability_unit m_avoid_relay;
	private:
		capability_unit* l[10];
		uint8_t max_units;
};

#endif
