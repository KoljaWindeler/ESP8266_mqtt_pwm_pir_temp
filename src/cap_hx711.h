#ifndef hx711_h
#define hx711_h

#include "main.h"
#include "HX711_c.h"

static constexpr char MQTT_HX711_TOPIC[]   = "hx711";
static constexpr char MQTT_HX711_TARE_TOPIC[]   = "hx711_tare";
#ifdef WITH_DISCOVERY
	static constexpr char MQTT_DISCOVERY_HX711_TOPIC[]      = "homeassistant/sensor/%s_hx711/config";
	static constexpr char MQTT_DISCOVERY_HX711_MSG[]      = "{\"name\":\"%s_hx711\", \"stat_t\": \"%s/r/hx711\"}";
#endif


class hx711 : public capability {
	public:
		hx711();
		~hx711();
		bool init();
		bool parse(uint8_t* config);
		uint8_t* get_key();

		//bool loop();

		uint8_t count_intervall_update();
		bool intervall_update(uint8_t slot);

		bool subscribe();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();

		//uint8_t* get_dep();
		//bool reg_provider(peripheral * p, uint8_t *source);
	private:
		HX711_c *p_hx;
		bool m_discovery_pub;
};


#endif