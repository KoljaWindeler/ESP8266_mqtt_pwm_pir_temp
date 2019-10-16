#ifndef button_h
	#define button_h


#include "main.h"
	#define BUTTON_INPUT_PIN       			0    // D3
	#define BUTTON_CHECK_INTERVALL 			1000 // ms
	#define BUTTON_FAST_CHECK_INTERVALL 100 // ms
	#define BUTTON_LONG_PUSH       			1000 // ms
	#define BUTTON_TIMEOUT         			1500 // max 1500ms timeout between each button press to count up (start of config)
	#define BUTTON_DEBOUNCE        			400  // ms debouncing for the botton
	#define BUTTON_RELEASE_OFFSET  			10   // internal use

	#define BUTTON_MODE_PUSH_BUTTON 1
	#define BUTTON_MODE_SWITCH 2

	#ifdef WITH_DISCOVERY
	static constexpr char MQTT_DISCOVERY_B_TOPIC[]      = "homeassistant/binary_sensor/%s_button/config";
	static constexpr char MQTT_DISCOVERY_B_MSG[]      = "{\"name\":\"%s_button\", \"stat_t\": \"%s/r/button\"}";
	static constexpr char MQTT_DISCOVERY_B1S_TOPIC[]      = "homeassistant/binary_sensor/%s_button1s/config";
	static constexpr char MQTT_DISCOVERY_B1S_MSG[]      = "{\"name\":\"%s_button1s\", \"stat_t\": \"%s/r/button1s\"}";
	#endif

	static constexpr char MQTT_BUTTON_TOPIC_0S[] = "button";   // publish
	static constexpr char MQTT_BUTTON_TOPIC_1S[] = "button1s"; // publish
	static constexpr char MQTT_BUTTON_TOPIC_2S[] = "button2s"; // publish
	static constexpr char MQTT_BUTTON_TOPIC_3S[] = "button3s"; // publish
	static constexpr char MQTT_BUTTON_RELEASE_TOPIC[] = "button_release"; // publish

	class button : public capability {
public:
		button();
		~button();
		bool init();
		bool loop();
		void interrupt();
		// bool intervall_update(uint8_t slot);
		// bool subscribe();
		// bool receive(uint8_t * p_topic, uint8_t * p_payload);
		bool parse(uint8_t * config);
		uint8_t * get_key();
		bool publish();
		bool consume_interrupt();
private:
		mqtt_parameter_8 m_state;
		uint8_t m_counter;
		uint32_t m_timer_button_down;
		uint32_t m_timer_debounce;
		uint32_t m_timer_checked;
		// uint8_t key[2];
		uint8_t m_pin;
		uint8_t m_mode_toggle_switch;
		bool m_pullup;
		bool m_polarity;
		bool m_pin_active;
		uint32_t m_interrupt_counter;
		bool m_discovery_pub;
		uint8_t m_ghost_avoidance;
		bool m_interrupt_ready;
	};


#endif // ifndef button_h
