#ifndef PIR_h
#define PIR_h

#include "main.h"

class PIR : public peripheral {
	public:
		PIR(uint8_t* k,uint8_t pin);
		~PIR();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		void interrupt();
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		uint8_t* get_key();
	private:
		bool publish();
		mqtt_parameter_8 m_state;
		uint8_t key[4];
		uint8_t m_pin;

};


#endif
