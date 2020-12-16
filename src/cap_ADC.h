#ifndef ADC_h
#define ADC_h

#include "main.h"
#define GPIO_D8          15 // D8
static constexpr char MQTT_ADC_TOPIC[]           = "adc";         // publish

#ifdef WITH_DISCOVERY
static constexpr char MQTT_DISCOVERY_ADC_TOPIC[]      = "homeassistant/sensor/%s_adc/config";
static constexpr char MQTT_DISCOVERY_ADC_MSG[]      = "{\"name\":\"%s_adc\", \"stat_t\": \"%s/r/adc\"}";
#endif

class ADC : public capability
{
	public:
		ADC();
		~ADC();
		bool init();
		bool intervall_update(uint8_t slot);
		bool parse(uint8_t* config);
		uint8_t count_intervall_update();
		//bool loop();
		//bool subscribe();
		//bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
		uint8_t* get_key();
	private:
		mqtt_parameter_8 m_state;
		bool m_discovery_pub;
};


#endif
