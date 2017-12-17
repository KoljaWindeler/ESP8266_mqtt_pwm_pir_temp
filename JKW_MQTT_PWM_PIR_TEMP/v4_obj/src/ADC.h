#ifndef ADC_h
#define ADC_h

#include "main.h"
#define GPIO_D8          15 // D8

class ADC : public peripheral
{
	public:
		ADC();
		~ADC();
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
		mqtt_parameter_8 m_state;
		uint8_t key[4];
};


#endif
