#ifndef PWM_h
#define PWM_h

#include "main.h"

class PWM : public peripheral {
	public:
		PWM();
		~PWM();
		bool init();
		bool loop();
		uint8_t count_intervall_update();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		void interrupt();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		uint8_t* get_key();
		bool publish();
		void setColor(uint8_t r, uint8_t g, uint8_t b);

		uint8_t getState(led* color);
		void setState(uint8_t value);
	private:
		bool publishLightState();
		bool publishLightBrightness();
		bool publishRGBColor();
		void updatePWMstate();
		mqtt_parameter_8 m_state;
		mqtt_parameter_8 m_light_brightness;
		mqtt_parameter_8 m_light_color;
		uint8_t key[4];
		led m_light_current;
	};


#endif
