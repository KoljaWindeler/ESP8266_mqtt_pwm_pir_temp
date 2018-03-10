#ifndef button_h
	#define button_h


#include "main.h"
	#define BUTTON_INPUT_PIN       0    // D3
	#define BUTTON_CHECK_INTERVALL 1000 // ms
	#define BUTTON_LONG_PUSH       1000 // ms
	#define BUTTON_TIMEOUT         1500 // max 1500ms timeout between each button press to count up (start of config)
	#define BUTTON_DEBOUNCE        400  // ms debouncing for the botton


	static constexpr char MQTT_BUTTON_TOPIC_0S[] = "button";   // publish
	static constexpr char MQTT_BUTTON_TOPIC_1S[] = "button1s"; // publish
	static constexpr char MQTT_BUTTON_TOPIC_2S[] = "button2s"; // publish
	static constexpr char MQTT_BUTTON_TOPIC_3S[] = "button3s"; // publish

	class button : public peripheral {
public:
		button();
		~button();
		bool init();
		bool loop();
		void interrupt();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		uint8_t count_intervall_update();
		bool receive(uint8_t * p_topic, uint8_t * p_payload);
		bool parse(uint8_t * config);
		uint8_t * get_key();
		bool publish();
private:
		mqtt_parameter_8 m_state;
		uint8_t m_counter;
		uint32_t m_timer_button_down;
		uint32_t m_timer_checked;
		uint8_t key[2];
	};


#endif // ifndef button_h
