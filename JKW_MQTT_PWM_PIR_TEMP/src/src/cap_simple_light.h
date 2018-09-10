#ifndef simple_light_h
#define simple_light_h

#include "main.h"
#define SIMPLE_LIGHT_PIN 12 // D6

class simple_light : public peripheral {
	public:
		simple_light();
		~simple_light();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		uint8_t* get_key();
		bool publish();
		void setColor(uint8_t r, uint8_t g, uint8_t b);
	private:
		mqtt_parameter_8 m_state;
		uint8_t key[3];

};


#endif
