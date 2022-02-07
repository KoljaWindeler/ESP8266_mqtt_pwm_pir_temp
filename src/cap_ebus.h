#ifndef ebus_h
#define ebus_h

#include "main.h"
#include "../lib/softserial/SoftwareSerial.h"

static constexpr char MQTT_HEATER_SET_TOPIC[] = "ebus_heater_set";
static constexpr char MQTT_WW_SET_TOPIC[]     = "ebus_ww_set";
static constexpr char MQTT_HEATER_REQ_TOPIC[] = "ebus_heater_req";
static constexpr char MQTT_ROOM_TOPIC[]       = "ebus_room";

#ifdef WITH_DISCOVERY
static constexpr char MQTT_DISCOVERY_EBUS_HEATER_SET_TOPIC[] = "homeassistant/sensor/%s_ebus_heater_set/config";
static constexpr char MQTT_DISCOVERY_EBUS_HEATER_SET_MSG[]   = "{\"name\":\"%s_ebus_heater_set\", \"stat_t\": \"%s/r/ebus_heater_set\"}";

static constexpr char MQTT_DISCOVERY_EBUS_HEATER_REQ_TOPIC[] = "homeassistant/sensor/%s_ebus_heater_req/config";
static constexpr char MQTT_DISCOVERY_EBUS_HEATER_REQ_MSG[]   = "{\"name\":\"%s_ebus_heater_req\", \"stat_t\": \"%s/r/ebus_heater_req\"}";

static constexpr char MQTT_DISCOVERY_EBUS_WW_SET_TOPIC[]     = "homeassistant/sensor/%s_ebus_ww_set/config";
static constexpr char MQTT_DISCOVERY_EBUS_WW_SET_MSG[]       = "{\"name\":\"%s_ebus_ww_set\", \"stat_t\": \"%s/r/ebus_ww_set\"}";

static constexpr char MQTT_DISCOVERY_EBUS_ROOM_TOPIC[]       = "homeassistant/sensor/%s_ebus_room/config";
static constexpr char MQTT_DISCOVERY_EBUS_ROOM_MSG[]         = "{\"name\":\"%s_ebus_room\", \"stat_t\": \"%s/r/ebus_room\"}";
#endif


#define EBUS_STATE_HEADER 0
#define EBUS_STATE_IGNORE 1
#define EBUS_STATE_PAYLOAD 2

// 30 F1 [05 07] [09] [BB] [04] ->[10 02]<- [00] [80] [FF] [79] [FF] F1 --> 0x10,0x02 -> 0x0210/256 = 33
static constexpr char EBUS_HEATER_SET[] = {0x30, 0xF1, 0x05, 0x07, 0x09}; 
static constexpr uint16_t EBUS_HEATER_SET_MAGIC[] = {5,2,2,16}; // 5 identifier, 2 ignore, 2x payload LSBF, divider 256
// 30 F1 [05 07] [09] [BB] [04] [10 02] [00] [80] [FF] ->[79]<- [FF] F1 --> 0x79 -> 0x79/2 = 39.5
static constexpr char EBUS_WW_SET[] = {0x30, 0xF1, 0x05, 0x07, 0x09}; 
static constexpr uint16_t EBUS_WW_SET_MAGIC[] = {5,7,1,2}; // 5 identifier, 7 ignore, 1x payload LSBF, divider 2
// 30 50 [50 14] [08] [61] ->[E6 20]<- 00 17 08  --> 0xE6,0x20 -> 0x20E6 / 256 = 32.8984
static constexpr char EBUS_HEATER_REQ_SET[] = {0x30, 0x50, 0x50, 0x14, 0x08}; 
static constexpr uint16_t EBUS_HEATER_REQ_SET_MAGIC[] = {5,1,2,256}; // 5 identifier, 1 ignore, 2x payload LSBF, divider 256
// 30 50 [50 14] [08] [61] [E6 20] ->[00 17]<- 08  --> 0x00,0x17 -> 0x1700 / 256 = 23.0
static constexpr char EBUS_ROOM[] = {0x30, 0x50, 0x50, 0x14, 0x08}; 
static constexpr uint16_t EBUS_ROOM_MAGIC[] = {5,3,2,256}; // 5 identifier, 1 ignore, 2x payload LSBF, divider 256

class ebus : public capability {
	public:
		ebus();
		~ebus();
		bool init();
		bool parse(uint8_t* config);
		uint8_t* get_key();
		bool loop();

		uint8_t count_intervall_update();
		bool intervall_update(uint8_t slot);
		//bool subscribe();
		//bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
		//uint8_t* get_dep();
		//bool reg_provider(peripheral * p, uint8_t *source);
	private:
		SoftwareSerial * swSer1;
		uint8_t m_rxPin;
		uint8_t m_txPin;
		uint8_t m_parse_state;
		mqtt_parameter_float m_state_heater_set;
		mqtt_parameter_float m_state_ww_set;
		mqtt_parameter_float m_state_heater_req_set;
		mqtt_parameter_float m_state_room;

		bool m_discovery_pub;
		char* identifier[4];
		uint16_t* identifier_magic[4];
		uint8_t m_dataset;
		uint8_t m_parser_state;
		uint8_t m_parser_data;
		uint8_t m_parser_ignore;
		uint8_t m_temp_buffer;

};

#endif
