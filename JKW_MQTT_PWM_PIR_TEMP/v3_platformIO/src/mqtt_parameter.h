#include <Arduino.h>
#ifndef mqttp_h
#define mqttp_h

class mqtt_parameter_8 {
	public:
		mqtt_parameter_8();
		void check_set(uint8_t input);
		void set(uint8_t input);
		boolean update_required;
		uint8_t value;
};

class mqtt_parameter_16 {
	public:
		mqtt_parameter_16();
		void check_set(uint16_t input);
		void set(uint16_t input);
		boolean update_required;
		uint16_t value;
};

class mqtt_parameter_32 {
	public:
		mqtt_parameter_32();
		void check_set(uint32_t input);
		void set(uint32_t input);
		boolean update_required;
		uint32_t value;
};

#endif
