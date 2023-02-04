#ifndef autarco_h
	#define autarco_h


#include "main.h"

	static constexpr char MQTT_AUTARCO_POWER_TOPIC[]       = "autarco_power"; // publish	
	static constexpr char MQTT_AUTARCO_TOTAL_KWH_TOPIC[]   = "autarco_kwh"; // publish	
	static constexpr char MQTT_AUTARCO_DAY_KWH_TOPIC[]     = "autarco_day_kwh"; // publish	
	static constexpr char MQTT_AUTARCO_TEMP_TOPIC[]        = "autarco_temp"; // publish	
	static constexpr char MQTT_AUTARCO_DC1_VOLT_TOPIC[]    = "autarco_dc1_v"; // publish	
	static constexpr char MQTT_AUTARCO_DC1_CURR_TOPIC[]    = "autarco_dc1_a"; // publish	
	static constexpr char MQTT_AUTARCO_DC2_VOLT_TOPIC[]    = "autarco_dc2_v"; // publish	
	static constexpr char MQTT_AUTARCO_DC2_CURR_TOPIC[]    = "autarco_dc2_a"; // publish	


	#define RS485_DIR_PIN 5    // D1
	#define RS485_INPUT_PIN 14 // D5
	#define RS485_OUPUT_PIN 4  // D2
	
	#define AUTARCO_STATE_IDLE 0
	#define AUTARCO_STATE_HEAD 1
	#define AUTARCO_STATE_CMD 2
	#define AUTARCO_STATE_LEN 3
	#define AUTARCO_STATE_PAYLOAD 4
	#define AUTARCO_STATE_CHECK_1 5
	#define AUTARCO_STATE_CHECK_2 6

	#define AUTARCO_MAX_BUFFER 80
	



	class autarco : public capability {
public:
		autarco();
		~autarco();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		uint8_t count_intervall_update();
		bool parse(uint8_t * config);
		uint8_t * get_key();
		bool publish();
private:
		void build_crc(uint8_t in);
		mqtt_parameter_16 m_power;
		mqtt_parameter_float m_dc1_volt;
		mqtt_parameter_float m_dc1_curr;
		mqtt_parameter_float m_dc2_volt;
		mqtt_parameter_float m_dc2_curr;
		mqtt_parameter_32 m_last_total_kwh;
		mqtt_parameter_float m_today_kwh;
		mqtt_parameter_float m_total_kwh;
		mqtt_parameter_float m_temp;


		char m_buffer[AUTARCO_MAX_BUFFER]; // 80 byte buffer max
		uint8_t m_slot;
		float m_kwh_sync;
		bool m_discovery_pub;
		uint8_t m_parser_state;
		uint8_t m_payload_len;
		uint8_t m_payload_i;
		uint16_t m_crc;
		uint16_t m_req_start_addr;
		uint32_t m_last_char_in;
		SoftwareSerial * swSer1;
	};


#endif // ifndef autarco_h
