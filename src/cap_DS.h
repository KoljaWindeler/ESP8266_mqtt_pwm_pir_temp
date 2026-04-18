#ifndef J_DS_h
#define J_DS_h

#include "main.h"
#include <OneWire.h>
#include <Wire.h>
#define DS_PIN           13 // D7
#define DS_MAX_SENSORS   10 // D7
#define MQTT_TEMPARATURE_TOPIC_N "temperature_%i"
static constexpr char MQTT_TEMPARATURE_COUNT_TOPIC[] = "temperature_count"; // publish


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
		float getDsTemp(uint8_t sensor);
		uint8_t getSensorCount();
	private:
		mqtt_parameter_8 m_state;
		uint8_t m_sensor_count;
		uint8_t m_sensor_addr[DS_MAX_SENSORS][8];
		OneWire* p_ds;
		uint8_t m_pin;
		bool m_discovery_pub;
};


#endif
