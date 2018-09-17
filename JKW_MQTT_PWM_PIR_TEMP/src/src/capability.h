#ifndef capability_h
#define capability_h

//#include <Arduino.h>
#include "main.h"

class capability {
	public:
		capability();
		bool parse_eeprom(uint8_t* e);
		bool parse(unsigned char* input, uint8_t* key, uint8_t* dep);
		bool parse(unsigned char* input, uint8_t* key);
		bool ensure_dep(unsigned char* input, uint8_t* dep);
		bool parse_wide(unsigned char* input, uint8_t* key_word, uint8_t* key_res);
		bool parse_wide(unsigned char* input, uint8_t* key_word, uint8_t* key_res, uint8_t* dep);
		bool parse_wide(unsigned char* input, const char* key_schema, uint8_t* key_word, uint8_t key_start, uint8_t key_end, uint8_t* key_res, uint8_t* dep);
};

extern capability cap;

#endif
