#ifndef button_h
#define button_h


#include "main.h"
#define BUTTON_INPUT_PIN 0  // D3

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
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool parse(uint8_t* config);
		uint8_t* get_key();
		bool publish();
	private:
		mqtt_parameter_8 m_state;
		uint8_t m_counter;
		uint32_t m_timer_button_down;
		uint8_t key[2];
};


#endif
