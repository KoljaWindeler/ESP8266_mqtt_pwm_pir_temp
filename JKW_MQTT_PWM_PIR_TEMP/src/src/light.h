#ifndef light_h
	#define light_h

#include "main.h"
#include "../lib/NeoPixelBus/src/internal/RgbColor.h"

	static constexpr char MQTT_LIGHT_TOPIC[]                          = "light";                          // get command here ON / OFF
	static constexpr char MQTT_LIGHT_COLOR_TOPIC[]                    = "light/color";                    // set value "0-99,0-99,0-99"  will switch hard to the value
	static constexpr char MQTT_LIGHT_BRIGHTNESS_TOPIC[]               = "light/brightness";               // set value 0-99 will switch hard to the value
	static constexpr char MQTT_LIGHT_DIMM_TOPIC[]                     = "light/dimm";                     // get ON/OFF command here
	static constexpr char MQTT_LIGHT_DIMM_BRIGHTNESS_TOPIC[]          = "light/dimm/brightness";          // set value, will dimm towards the new value
	static constexpr char MQTT_LIGHT_DIMM_DELAY_TOPIC[]               = "light/dimm/delay";               // set value, will dimm towards the new value
	static constexpr char MQTT_LIGHT_DIMM_COLOR_TOPIC[]               = "light/dimm/color";               // set value "0-99,0-99,0-99", will dimm towards the new value
	static constexpr char MQTT_LIGHT_ANIMATION_BRIGHTNESS_TOPIC[]     = "light/animation/brightness";     // 0..99
	static constexpr char MQTT_LIGHT_ANIMATION_RAINBOW_TOPIC[]        = "light/animation/rainbow";        // get command here ON / OFF
	static constexpr char MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_TOPIC[] = "light/animation/simple_rainbow"; // get command here ON / OFF
	static constexpr char MQTT_LIGHT_ANIMATION_COLOR_WIPE_TOPIC[]     = "light/animation/color_wipe";     // get command here ON / OFF

	#define T_SL                     1
	#define T_PWM                    2
	#define T_NEO                    3
	#define T_BOne                   4
	#define T_AI                     5

	#define ANIMATION_STEP_TIME      15 // 256 steps per rotation * 15 ms/step = 3.79 sec pro rot
	#define NEOPIXEL_LED_COUNT       24
	#define ANIMATION_OFF            0
	#define ANIMATION_RAINBOW_WHEEL  1
	#define ANIMATION_RAINBOW_SIMPLE 2
	#define ANIMATION_COLOR_WIPE     3

	#define STATE_ONFOR 						"ONFOR"


	class light : public peripheral {
public:
		light();
		~light();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t * config);
		uint8_t count_intervall_update();
		bool receive(uint8_t * p_topic, uint8_t * p_payload);
		bool publish();
		uint8_t * get_key();

		bool reg_provider(peripheral * p, uint8_t type);
		void send_current_light();
		void setColor(uint8_t r, uint8_t g, uint8_t b);
		void toggle();
		void setState(bool state);
private:
		mqtt_parameter_8 m_state;                // on / off
		mqtt_parameter_32 m_light_color;         // r,g,b code
		mqtt_parameter_8 m_light_brightness;     // placebo :)
		mqtt_parameter_8 m_animation_type;       // type 1 = rainbow wheel, 2 = simple rainbow .. see define above
		mqtt_parameter_8 m_animation_brightness; // placebo :)

		uint32_t timer_dimmer;
		uint32_t timer_dimmer_start;
		uint32_t timer_dimmer_end;

		led m_light_target;
		led m_light_start;
		led m_light_current; // actual value written to PWM
		led m_light_backup;  // to be able to resume "dimm on" to the last set color

		uint16_t m_pwm_dimm_time; // 10ms per Step, 255*0.01 = 2.5 sec
		uint8_t key[4];
		peripheral * provider;
		uint8_t type;
		uint8_t m_animation_pos; // pointer im wheel
		uint32_t m_animation_dimm_time;
		uint32_t m_onfor_offtime;

		bool publishRGBColor();
		bool publishLightBrightness();
		bool publishLightState();
		bool publishAnimationType();
		bool publishAnimationBrightness();

		void setAnimationType(int type);
		void DimmTo(led dimm_to);
		RgbColor Wheel(byte WheelPos);
	};


#endif // ifndef light_h
