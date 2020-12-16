#ifndef tuya_bridge_h
#define tuya_bridge_h


#include "main.h"

static constexpr char MQTT_TUYA_BRIDGE_BATTERY_TOPIC[] = "TUYA_BAT";
static constexpr char MQTT_TUYA_BRIDGE_REED_TOPIC[]    = "TUYA_REED";
static constexpr char MQTT_TUYA_BRIDGE_WAKE_TOPIC[]		 = "TUYA_WAKE";

#define RFB_START_0 10
#define RFB_START_1 11
#define RFB_VERSION 12
#define RFB_CMD     13
#define RFB_LEN_1   14
#define RFB_LEN_2   15
#define RFB_DATA    16
#define RFB_CHK     17

#define RFB_TIMEOUT 1000

struct luminea_mcu {
	uint8_t cmd;
	uint16_t len;
	uint8_t data[40];
	uint8_t data_p;
	uint8_t chk;
	uint8_t rdy;
};

struct luminea_status {
	uint8_t reed;
	uint8_t tamper;
	uint8_t batt;
};

class tuya_bridge : public capability {
	public:
		tuya_bridge();
		~tuya_bridge();
		bool init();
		bool loop();
		void interrupt();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool parse(uint8_t* config);
		uint8_t* get_key();
		bool publish();
	private:
		void send_cmd(uint8_t cmd, uint16_t len, uint8_t* data);
		uint8_t m_recv_state;
		uint8_t m_msg_state;
		uint32_t rfb_last_received;
		uint8_t key[4];
		luminea_mcu rfb;
		luminea_status status;
		uint8_t t; // temporary serial byte
		mqtt_parameter_8 m_battery;
		mqtt_parameter_8 m_state;

};


#endif
