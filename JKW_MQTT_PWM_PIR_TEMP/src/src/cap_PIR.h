#ifndef PIR_h
#define PIR_h

#include "main.h"
static constexpr char MQTT_MOTION_TOPIC[]        = "motion_%i";      // publish

#ifdef WITH_DISCOVERY
static constexpr char MQTT_DISCOVERY_M_TOPIC[]      = "homeassistant/binary_sensor/%s_motion_%i/config";
static constexpr char MQTT_DISCOVERY_M_MSG[]      = "{\"name\":\"%s_motion_%i\", \"stat_t\": \"%s/r/motion_%i\"}";
#endif

class PIR_SENSOR {
	public:
		PIR_SENSOR(uint8_t pin, boolean invert);
		~PIR_SENSOR();
		void attache_member(PIR_SENSOR* new_next);
		bool init();
		bool interrupt();
		bool publish();
		uint8_t get_pin();
		PIR_SENSOR* get_next();
	private:
		mqtt_parameter_8 m_state;
		uint8_t m_pin;
		uint8_t m_invert;
		boolean m_init_done;
		boolean m_discovery_pub;
		PIR_SENSOR* m_next;
		uint8_t m_id;
};

class PIR : public capability {
	public:
		PIR();
		~PIR();
		bool init();
		// bool loop();
		// bool intervall_update(uint8_t slot);
		// bool subscribe();
		bool parse(uint8_t* config);
		void interrupt();
		// bool receive(uint8_t* p_topic, uint8_t* p_payload);
		uint8_t* get_key();
	private:
		bool publish();
		mqtt_parameter_8 m_state;
		uint8_t key[4];
		uint8_t m_pin;
		bool init_done;
		bool m_discovery_pub;
		PIR_SENSOR* m_list;
};

#endif
