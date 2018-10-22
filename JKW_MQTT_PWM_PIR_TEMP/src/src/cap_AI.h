#ifndef AI_h
#define AI_h

#include "main.h"
#include <my92x1.h>


#define AI_DI_PIN          13 // mtdi 12
#define AI_DCKI_PIN        15 // mtms gpio 14?

class AI : public peripheral {
	public:
		AI();
		~AI();
		bool init();
		// bool loop();

		// bool subscribe();
		bool parse(uint8_t* config);
		// bool receive(uint8_t* p_topic, uint8_t* p_payload);
		// bool publish();
		uint8_t* get_key();
		my92x1* getmy929x1();
		void setColor(uint8_t r, uint8_t g, uint8_t b);
		uint8_t getState(led* color);
		uint8_t* get_dep();
		//my9291* my9291;
	private:
		mqtt_parameter_8 m_state;
		uint8_t key[3];
		led m_light_current;
		my92x1 _my92x1;
};

#endif
