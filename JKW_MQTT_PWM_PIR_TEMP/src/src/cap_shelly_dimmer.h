#ifndef shelly_dimmer_h
#define shelly_dimmer_h


#include "main.h"

static constexpr char MQTT_SHELLY_DIMMER_POWER_TOPIC[] = "SHD_POWER";

#define SHELLY_DIMMER_POWER_UPDATE_RATE 30000UL // 30 sec

#define SHD_START   10
#define SHD_ID      11
#define SHD_CMD     12
#define SHD_LEN     13
#define SHD_DATA    14
#define SHD_CHK_1   15
#define SHD_CHK_2   16
#define SHD_END     17


struct shelly_dimmer_msg {
	uint8_t start;
	uint8_t id;
    uint8_t cmd;
	uint8_t len;
	uint8_t data[72];
    uint8_t data_p;
	uint16_t chk;
	uint8_t end;
};

class shelly_dimmer : public capability {
	public:
		shelly_dimmer();
		~shelly_dimmer();
		bool init();
		bool loop();
		void interrupt();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool parse(uint8_t* config);
		uint8_t* get_key();
        uint8_t* get_dep();
		bool publish();
        void setColor(uint8_t b);
	private:
		void send_cmd(uint8_t cmd,uint8_t len,uint8_t *payload);
        void send_cmd(uint8_t cmd,uint8_t len,uint8_t *payload, boolean debug);
		uint8_t m_recv_state;
		shelly_dimmer_msg m_msg_in;
        shelly_dimmer_msg m_msg_out;
		uint8_t t; // temporary serial byte
		mqtt_parameter_16 m_power;
        uint32_t m_last_char_recv;
};


#endif
