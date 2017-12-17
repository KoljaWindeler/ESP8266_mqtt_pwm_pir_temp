#ifndef light_h
#define light_h

#include "main.h"
#include "../lib/NeoPixelBus/src/internal/RgbColor.h"

class light : public peripheral {
	public:
		light();
		~light();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
		uint8_t* get_key();

		bool reg_provider(peripheral* p,uint8_t type);
		void send_current_light();
		void setColor(uint8_t r, uint8_t g, uint8_t b);
		void toggle();
		void setState(bool state);
	private:
		mqtt_parameter_8 m_state;							// on / off
		mqtt_parameter_32 m_light_color; 			// r,g,b code
		mqtt_parameter_8 m_light_brightness;	// placebo :)
		mqtt_parameter_8 m_animation_type; 		// type 1 = rainbow wheel, 2 = simple rainbow .. see define above
		mqtt_parameter_8 m_animation_brightness;	// placebo :)

		uint32_t timer_dimmer;
		uint32_t timer_dimmer_start;
		uint32_t timer_dimmer_end;

		led m_light_target;
		led m_light_start;
		led m_light_current; // actual value written to PWM
		led m_light_backup;  // to be able to resume "dimm on" to the last set color

		uint16_t m_pwm_dimm_time;    // 10ms per Step, 255*0.01 = 2.5 sec
		uint8_t key[4];
		peripheral* provider;
		uint8_t type;
		uint8_t m_animation_pos;     // pointer im wheel
		uint32_t m_animation_dimm_time;

		bool publishRGBColor();
		bool publishLightBrightness();
		bool publishLightState();
		bool publishAnimationType();
		bool publishAnimationBrightness();

		void setAnimationType(int type);
		void DimmTo(led dimm_to);
		RgbColor Wheel(byte WheelPos);
};


#endif
