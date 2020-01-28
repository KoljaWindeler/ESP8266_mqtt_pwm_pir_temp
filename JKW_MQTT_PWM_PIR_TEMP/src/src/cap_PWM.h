#ifndef PWM_h
#define PWM_h

#include "main.h"

class PWM : public light_providing_capability {
	public:
		PWM(uint8_t* k, uint8_t pin0,uint8_t pin1, uint8_t pin2);
		PWM(uint8_t* k, uint8_t pin0,uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4);
		~PWM();
		bool init();
		// bool loop();
		// bool intervall_update(uint8_t slot);
		// bool subscribe();
		bool parse(uint8_t* config);
		// bool receive(uint8_t* p_topic, uint8_t* p_payload);
		uint8_t* get_key();
		uint8_t* get_dep();
		bool publish();

		uint8_t getState(led* color);
		void setState(uint8_t value);

		// light providing capability specific calls
		void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t px);
		uint8_t get_modes();
		void print_name();

	private:
		bool publishLightState();
		bool publishLightBrightness();
		bool publishRGBColor();
		void updatePWMstate();
		mqtt_parameter_8 m_state;
		mqtt_parameter_8 m_light_brightness;
		mqtt_parameter_8 m_light_color;
		uint8_t key[4];
		uint8_t dep[4];
		led m_light_current;
		uint8_t m_pins[5];
		bool m_discovery_pub;
	};


#endif
