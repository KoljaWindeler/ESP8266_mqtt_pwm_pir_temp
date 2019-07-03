#ifndef simple_light_h
#define simple_light_h

#include "main.h"
#define SIMPLE_LIGHT_PIN 12 // D6

#ifdef WITH_DISCOVERY
static constexpr char MQTT_DISCOVERY_SL_TOPIC[]      = "homeassistant/light/%s/config";
static constexpr char MQTT_DISCOVERY_SL_MSG[]      = "{\"name\":\"%s\",\"stat_t\":\"%s/r/light\",\"cmd_t\":\"%s/s/light\",\"ret\":\"true\"}";
#endif

class simple_light : public capability {
	public:
		simple_light();
		~simple_light();
		bool init();
		// bool loop();
		// bool intervall_update(uint8_t slot);
		// bool subscribe();
		bool parse(uint8_t* config);
		// bool receive(uint8_t* p_topic, uint8_t* p_payload);
		uint8_t* get_key();
		bool publish();
		void setColor(uint8_t r, uint8_t g, uint8_t b);
		uint8_t* get_dep();
	private:
		mqtt_parameter_8 m_state;
		uint8_t m_pin;
		bool m_discovery_pub;

};


#endif
