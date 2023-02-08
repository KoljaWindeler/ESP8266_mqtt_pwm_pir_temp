#ifdef WITH_IR
#ifndef ir_h
#define ir_h

#include "main.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>


static constexpr char MQTT_IR_TOPIC[]   = "ir_recv";
#ifdef WITH_DISCOVERY
	static constexpr char MQTT_DISCOVERY_IR_TOPIC[]      = "homeassistant/sensor/%s_ir/config";
	static constexpr char MQTT_DISCOVERY_IR_MSG[]      = "{\"name\":\"%s_ir\", \"stat_t\": \"%s/r/ir_recv\"}";
#endif

class ir : public capability {
	public:
		ir();
		~ir();
		bool init();
		bool parse(uint8_t* config);
		uint8_t* get_key();

		bool loop();

		//uint8_t count_intervall_update();
		//bool intervall_update(uint8_t slot);

		//bool subscribe();
		//bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();

		//uint8_t* get_dep();
		//bool reg_provider(peripheral * p, uint8_t *source);
	private:
		IRrecv* pMyReceiver;
 		decode_results* pResults;
		uint8_t m_pin;
		mqtt_parameter_64 m_state;
		bool m_discovery_pub;
};


#endif
#endif
