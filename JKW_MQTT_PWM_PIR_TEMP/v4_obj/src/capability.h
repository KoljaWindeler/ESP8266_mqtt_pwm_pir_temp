#ifndef capability_h
#define capability_h

#include <Arduino.h>

class capability {
	public:
		capability();
		bool parse_eeprom(uint8_t* e);
		bool parse(unsigned char* input, uint8_t* key);
};

#endif
