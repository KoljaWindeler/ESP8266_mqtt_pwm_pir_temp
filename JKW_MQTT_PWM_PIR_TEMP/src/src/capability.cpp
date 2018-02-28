#include "capability.h"

// 342094
// 341550
capability_unit::capability_unit(){ };

capability::capability(){
	max_units = 0;

	reg(&m_use_neo_as_rgb); // if true we're going to send the color to the neopixel and not to the pwm pins
	m_use_neo_as_rgb.key = 'N';

	reg(&m_use_my92x1_as_rgb); // for b1 bubble / aitinker
	m_use_my92x1_as_rgb.key = 'M';

	reg(&m_use_pwm_as_rgb); // for mosfet pwm
	m_use_pwm_as_rgb.key = 'P';

	reg(&m_avoid_relay); // to avoid clicking relay
	m_avoid_relay.key = 'R';
};

void capability::reg(capability_unit * u){
	l[max_units] = u;
	u->active    = false;
	max_units    = (max_units + 1) % (sizeof(l) / sizeof(l[0]));
	if (max_units == 0) {
		Serial.println(F("WARNING, to many capability_units"));
	}
}

bool capability::parse(unsigned char * input){
	uint8_t key;
	uint8_t state = 0;

	while (*input) {
		if (state == 0) {
			key = *input;
			state++;
		} else if (state == 1) {
			if (*input != '=') {
				return false;
			} else {
				state++;
			}
		} else if (state == 2) {
			if (*input == ',') {
				state = 0;
			} else {
				for (uint8_t i = 0; i < max_units; i++) {
					if (l[i]->key == key) {
						l[i]->active = *input - '0';
						break;
					}
				}
			}
		}
		input++;
	}
}
