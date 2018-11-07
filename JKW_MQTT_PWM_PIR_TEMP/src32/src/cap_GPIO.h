#ifndef J_GPIO_h
	#define J_GPIO_h

	#define PWM_MAX 99
	#define PWM_MIN 0
	#define PWM_ON  255
	#define PWM_OFF 254

#include "main.h"

	static constexpr char MQTT_J_GPIO_OUTPUT_STATE_TOPIC[]      = "gpio_%i_state";
	static constexpr char MQTT_J_GPIO_OUTPUT_DIMM_TOPIC[]       = "gpio_%i_dimm";
	static constexpr char MQTT_J_GPIO_OUTPUT_BRIGHTNESS_TOPIC[] = "gpio_%i_brightness";
	static constexpr char MQTT_J_GPIO_INPUT_HOLD_TOPIC[]       = "gpio_%i_hold";
	static constexpr char MQTT_J_GPIO_TOGGLE_TOPIC[] = "gpio_%i_toggle";
	static constexpr char MQTT_J_GPIO_PULSE_TOPIC[]  = "gpio_%i_pulse";


	class dimmer {
public:
		dimmer(uint8_t gpio, bool invers);
		bool loop();
		bool set_brightness(uint8_t t);
		bool set_brightness(uint8_t t, bool dimming);
		void set_state(bool s);
		void dimm_to(uint8_t t);
		void dimm_to(uint8_t t, bool dimming);
private:
		uint8_t m_gpio        = 0;
		bool m_invers         = false;
		uint8_t m_start_v     = 0;
		uint8_t m_target_v    = 0;
		uint8_t m_current_v   = 0;
		uint8_t m_backup_v    = 0;
		uint32_t m_next_time   = 0;
		uint32_t m_end_time   = 0;
		uint32_t m_start_time = 0;
		uint8_t m_step_time   = 0;
	};

	class J_GPIO : public capability {
public:
		J_GPIO();
		~J_GPIO();
		bool init();
		bool loop();
		// bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t * config);
		bool receive(uint8_t * p_topic, uint8_t * p_payload);
		bool publish();
		uint8_t * get_key();
private:
		void set_output(uint8_t pin, uint8_t state);
		bool m_pin_out[17]    = { false };
		dimmer * m_dimmer[17] = { NULL };
		bool m_pin_in[17]     = { false };
		mqtt_parameter_8 m_state[17];
		mqtt_parameter_8 m_brightness[17];
		uint32_t m_timing_parameter[17] = { 0 };
		bool m_invert[17];
	};


#endif // ifndef J_GPIO_h
