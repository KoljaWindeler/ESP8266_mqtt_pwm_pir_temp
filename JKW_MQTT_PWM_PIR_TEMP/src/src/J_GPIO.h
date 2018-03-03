#ifndef J_GPIO_h
#define J_GPIO_h

#include "main.h"

static constexpr char MQTT_J_GPIO_OUTPUT_TOPIC[]   = "gpio_%i_out";
static constexpr char MQTT_J_GPIO_TOGGLE_TOPIC[]   = "gpio_%i_toggle";
static constexpr char MQTT_J_GPIO_PULSE_TOPIC[]    = "gpio_%i_pulse";

class J_GPIO : public peripheral {
	public:
		J_GPIO();
		~J_GPIO();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
		uint8_t* get_key();
	private:
		void set_output(uint8_t pin, uint8_t state);
		bool m_pin[17]={false};
		mqtt_parameter_8 m_state[17];
		uint32_t m_next_action[17]={0};
};


#endif
