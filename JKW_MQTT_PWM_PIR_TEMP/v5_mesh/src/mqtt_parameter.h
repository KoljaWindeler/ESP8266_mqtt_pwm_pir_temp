#ifndef mqttp_h
#define mqttp_h
#include <Arduino.h>

class mqtt_parameter_8 {
	public:
		mqtt_parameter_8();
		void check_set(uint8_t input);
		void set(uint8_t input);
		void set(uint8_t input, bool update);
		uint8_t get_value();
		void outdated();
		void outdated(bool update_required);
		bool get_outdated();
	private:
		uint8_t _value;
		boolean _update_required;

};

class mqtt_parameter_16 {
	public:
		mqtt_parameter_16();
		void check_set(uint16_t input);
		void set(uint16_t input);
		void set(uint16_t input, bool update);
		uint16_t get_value();
		void outdated();
		void outdated(bool update_required);
		bool get_outdated();
	private:
		uint16_t _value;
		boolean _update_required;
};

class mqtt_parameter_32 {
	public:
		mqtt_parameter_32();
		void check_set(uint32_t input);
		void set(uint32_t input);
		void set(uint32_t input, bool update);
		uint32_t get_value();
		void outdated();
		void outdated(bool update_required);
		bool get_outdated();
	private:
		uint32_t _value;
		boolean _update_required;
};

#endif
