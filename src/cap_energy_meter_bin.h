#ifndef energy_meter_bin_h
	#define energy_meter_bin_h


#include "main.h"
#include "../lib/softserial/SoftwareSerial.h"

/*
[77] [07 01 00 01 08 00 FF] [65 00 1C 5D 04 01] [62 1E] [52 FF] [69] 00 00 00 00 00 00 32 12 [01] 
												FF = -1
												0x3212 = 12818/10 = 1281.8 wh = 1.2818 kwh (bezogen)

[77] [07 01 00 02 08 00 FF] [01] [01] [62 1E] [52 FF] [69] 00 00 00 00 00 00 A4 A8 [01]
												FF = -1 = /10 .. wh! 
												0xa4a8 = 42152/10 = 4215.2Wh = 4.2152 kwh (GELIEFERT)

[77] [07 01 00 10 07 00 FF] [01] [01] [62 1B] [52 00] [55] [FF FF FE EC] [01]
	aktuelle leistung                           00 = 10 hoch 0
												0xFFFFFEEC = -113

http://www.stefan-weigert.de/php_loader/sml.php
*/


	static constexpr char MQTT_ENERGY_METER_BIN_TOTAL_GRID_TOPIC[]    = "em_tot_grid"; // publish
	static constexpr char MQTT_ENERGY_METER_BIN_TOTAL_SOLAR_TOPIC[]    = "em_tot_solar"; // publish
	static constexpr char MQTT_ENERGY_METER_BIN_CUR_TOPIC[] = "em_cur"; // publish
	static constexpr char MQTT_ENERGY_METER_BIN_CUR_FAST_TOPIC[] = "em_cur_fast"; // publish
	
	static constexpr char ENERGY_METER_BIN_TOTAL_GRID_BIN[] = {0x77, 0x07, 0x01, 0x00, 0x01, 0x08, 0x00, 0xff}; 
	static constexpr char ENERGY_METER_BIN_TOTAL_GRID_BIN_MAGIC[] = {8,9,1,1,8}; // 8 identifier, 9 ignore, 1xscale, 1x ignore, 8x payload
	static constexpr char ENERGY_METER_BIN_TOTAL_SOLAR_BIN[] = {0x77, 0x07, 0x01, 0x00, 0x02, 0x08, 0x00, 0xff}; 
	static constexpr char ENERGY_METER_BIN_TOTAL_SOLAR_BIN_MAGIC[] = {8,5,1,1,8}; // 8 identifier, 5 ignore, 1xscale, 1x ignore, 8x payload
	static constexpr char ENERGY_METER_BIN_CUR_BIN[]   = {0x77, 0x07, 0x01, 0x00, 0x10, 0x07, 0x00, 0xff}; 
	static constexpr char ENERGY_METER_BIN_CUR_BIN_MAGIC[] = {8,5,1,1,4}; // 8, 5 ignore, sclae, 1xignore, 4x payload

#ifdef WITH_DISCOVERY
	static constexpr char MQTT_DISCOVERY_EM_BIN_CUR_TOPIC[]      = "homeassistant/sensor/%s_em_cur/config";
	static constexpr char MQTT_DISCOVERY_EM_BIN_CUR_MSG[]      = "{\"name\":\"%s_em_cur\", \"stat_t\": \"%s/r/em_cur\"}";
	static constexpr char MQTT_DISCOVERY_EM_BIN_CUR_FAST_TOPIC[]      = "homeassistant/sensor/%s_em_cur_fast/config";
	static constexpr char MQTT_DISCOVERY_EM_BIN_CUR_FAST_MSG[]      = "{\"name\":\"%s_em_cur_fast\", \"stat_t\": \"%s/r/em_cur_fast\"}";
	static constexpr char MQTT_DISCOVERY_EM_BIN_TOTAL_GRID_TOPIC[]      = "homeassistant/sensor/%s_em_tot_grid/config";
	static constexpr char MQTT_DISCOVERY_EM_BIN_TOTAL_GRID_MSG[]      = "{\"name\":\"%s_em_tot_grid\", \"stat_t\": \"%s/r/em_tot_grid\"}";
	static constexpr char MQTT_DISCOVERY_EM_BIN_TOTAL_SOLAR_TOPIC[]      = "homeassistant/sensor/%s_em_tot_solar/config";
	static constexpr char MQTT_DISCOVERY_EM_BIN_TOTAL_SOLAR_MSG[]      = "{\"name\":\"%s_em_tot_solar\", \"stat_t\": \"%s/r/em_tot_solar\"}";
#endif

	#define EM_STATE_WAIT 0
	#define EM_STATE_IGNORE 1
	#define EM_STATE_SCALE 2
	#define EM_STATE_IGNORE_2 3
	#define EM_STATE_PAYLOAD 4
	#define EM_STATE_CHECK 5


	class energy_meter_bin : public capability {
public:
		energy_meter_bin();
		~energy_meter_bin();
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
		char m_total_grid_kwh[16]; // 6 vor, 8 nachkomma
		char m_total_solar_kwh[16]; // 6 vor, 8 nachkomma
		char temp[16];
		SoftwareSerial * swSer1;
		uint8_t dataset;
		float storage[2];
		uint8_t m_freq;
		uint8_t m_slot;
		char* identifier[3];
		char* identifier_magic[3];
		bool m_discovery_pub;

		uint8_t m_parser_state;
		uint8_t m_parser_i;
		uint8_t m_parser_ignore;
		float m_parser_scale;
	};


#endif // ifndef energy_meter_bin_h
