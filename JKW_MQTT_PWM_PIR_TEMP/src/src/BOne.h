#ifndef BOne_h
#define BOne_h

#include "main.h"
#include <my92x1.h>

#define MY9291_DI_PIN          12 // mtdi 12
#define MY9291_DCKI_PIN        14 // mtms gpio 14?

class BOne : public peripheral {
	public:
		BOne();
		~BOne();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
		uint8_t* get_key();
		my92x1* getmy929x1();
		void setColor(uint8_t r, uint8_t g, uint8_t b);
		uint8_t getState(led* color);
	private:
		mqtt_parameter_8 m_state;
		uint8_t key[3];
		led m_light_current;
		my92x1 _my92x1_b1;
};


#endif
