#ifndef J_DHT22_h
#define J_DHT22_h

#include "main.h"
#include <DHT.h>
#define DHT_PIN          2  // D4

#ifdef WITH_DISCOVERY
static constexpr char MQTT_DISCOVERY_DHT_TEMP_TOPIC[]      = "homeassistant/sensor/%s_temperature_DHT/config";
static constexpr char MQTT_DISCOVERY_DHT_TEMP_MSG[]      = "{\"name\":\"%s_temperature\", \"stat_t\": \"%s/r/temperature\"}";
static constexpr char MQTT_DISCOVERY_DHT_HUM_TOPIC[]      = "homeassistant/sensor/%s_humidity_DHT/config";
static constexpr char MQTT_DISCOVERY_DHT_HUM_MSG[]      = "{\"name\":\"%s_humidity\", \"stat_t\": \"%s/r/humidity\"}";
#endif

class J_DHT22 : public capability {
	public:
		J_DHT22();
		~J_DHT22();
		bool init();
		bool intervall_update(uint8_t slot);
		uint8_t count_intervall_update();
		bool parse(uint8_t* config);
		uint8_t* get_key();
		// bool loop();
		// bool subscribe();
		// bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
	private:
		DHT* p_dht;
		uint8_t m_pin;
		bool m_discovery_pub;
// uint8_t key[4];
};


#endif
