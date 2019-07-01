#ifndef J_DS_h
#define J_DS_h

#include "main.h"
#include <OneWire.h>
#include <Wire.h>
#define DS_PIN           13 // D7
static constexpr char MQTT_DISCOVERY_DS_TOPIC[]      = "homeassistant/sensor/%s_temperature_DS/config";
static constexpr char MQTT_DISCOVERY_DS_MSG[]      = "{\"name\":\"%s_temperature\", \"stat_t\": \"%s/r/temperature\"}";

class J_DS : public capability {
	public:
		J_DS();
		~J_DS();
		bool init();
		bool parse(uint8_t* config);
		uint8_t* get_key();
		uint8_t count_intervall_update();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		// bool loop();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
	private:
		mqtt_parameter_8 m_state;
		OneWire* p_ds;
		uint8_t m_pin;
		bool m_discovery_pub;
		float getDsTemp();
};


#endif
