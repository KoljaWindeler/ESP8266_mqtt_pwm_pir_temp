#ifndef uptime_h
#define uptime_h

#include "main.h"

static constexpr char MQTT_UPTIME_TOPIC[]   = "uptime";
static constexpr char MQTT_UPTIME_RESET_TOPIC[]   = "uptime_reset";


class uptime : public capability {
	public:
		uptime();
		~uptime();
		bool init();
		// bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
		uint8_t* get_key();
	private:
		bool m_active_high;
		uint8_t m_pin;
		mqtt_parameter_16 m_counter;
};


#endif
