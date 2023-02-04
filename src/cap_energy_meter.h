#ifdef WITH_EM
#ifndef energy_meter_h
	#define energy_meter_h


#include "main.h"
	#include "../lib/softserial/SoftwareSerial.h"

	static constexpr char MQTT_ENERGY_METER_TOTAL_TOPIC[]    = "em_tot"; // publish
	static constexpr char MQTT_ENERGY_METER_CUR_TOPIC[] = "em_cur"; // publish
	static constexpr char MQTT_ENERGY_METER_CUR_FAST_TOPIC[] = "em_cur_fast"; // publish
	static constexpr char MQTT_ENERGY_METER_CUR_TOPIC_L[] = "em_cur_l%i"; // publish

	static constexpr char ENERGY_METER_TOTAL[]     = "1-0:1.8.0*255("; // publish
	static constexpr char ENERGY_METER_CUR[]       = "1-0:16.7.0*255("; // publish
	static constexpr char ENERGY_METER_CUR_L1[]    = "1-0:36.7.0*255("; // publish
	static constexpr char ENERGY_METER_CUR_L2[]    = "1-0:56.7.0*255("; // publish
	static constexpr char ENERGY_METER_CUR_L3[]    = "1-0:76.7.0*255("; // publish

#ifdef WITH_DISCOVERY
	static constexpr char MQTT_DISCOVERY_EM_CUR_TOPIC[]      = "homeassistant/sensor/%s_em_cur/config";
	static constexpr char MQTT_DISCOVERY_EM_CUR_MSG[]      = "{\"name\":\"%s_em_cur\", \"stat_t\": \"%s/r/em_cur\"}";
	static constexpr char MQTT_DISCOVERY_EM_CUR_FAST_TOPIC[]      = "homeassistant/sensor/%s_em_cur_fast/config";
	static constexpr char MQTT_DISCOVERY_EM_CUR_FAST_MSG[]      = "{\"name\":\"%s_em_cur_fast\", \"stat_t\": \"%s/r/em_cur_fast\"}";
	static constexpr char MQTT_DISCOVERY_EM_CUR_Li_TOPIC[]      = "homeassistant/sensor/%s_em_cur_l$i/config";
	static constexpr char MQTT_DISCOVERY_EM_CUR_Li_MSG[]      = "{\"name\":\"%s_em_cur_l%i\", \"stat_t\": \"%s/r/em_cur_l%i\"}";
	static constexpr char MQTT_DISCOVERY_EM_TOTAL_TOPIC[]      = "homeassistant/sensor/%s_em_tot/config";
	static constexpr char MQTT_DISCOVERY_EM_TOTAL_MSG[]      = "{\"name\":\"%s_em_tot\", \"stat_t\": \"%s/r/em_tot\"}";
#endif

	#define ENERGY_METER_ID_TOTAL 0
	#define ENERGY_METER_ID_CUR 1
	#define ENERGY_METER_ID_CUR_L1 2
	#define ENERGY_METER_ID_CUR_L2 3
	#define ENERGY_METER_ID_CUR_L3 4


	class energy_meter : public capability {
public:
		energy_meter();
		~energy_meter();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		uint8_t count_intervall_update();
		bool parse(uint8_t * config);
		uint8_t * get_key();
		bool publish();
private:
		mqtt_parameter_16 m_state;
		char m_current_w[10]; // 6 vor, 2 nachkomma
		char m_current_w_l1[10]; // 6 vor, 2 nachkomma
		char m_current_w_l2[10]; // 6 vor, 2 nachkomma
		char m_current_w_l3[10]; // 6 vor, 2 nachkomma
		char m_total_kwh[16]; // 6 vor, 8 nachkomma
		char temp[16];
		SoftwareSerial * swSer1;
		uint8_t dataset;
		uint8_t m_freq;
		uint8_t m_slot;
		char* identifier[5];
		bool m_discovery_pub;
	};


#endif // ifndef energy_meter_h

#endif