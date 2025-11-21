#ifndef autarco_hybrid_h
	#define autarco_hybrid_h


#include "main.h"

	static constexpr char MQTT_AUTARCO_HYBRID_POWER_TOPIC[]       = "autarco_power"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_BATTERY_POWER_TOPIC[]    = "autarco_battery_power"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_DC_POWER_TOPIC[]    = "autarco_dc_power"; // publish
	static constexpr char MQTT_AUTARCO_HYBRID_METER_POWER_TOPIC[]    = "autarco_grid_meter_power"; // publish
	static constexpr char MQTT_AUTARCO_HYBRID_AC_POWER_TOPIC[]    = "autarco_ac_power"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_HOUSE_POWER_TOPIC[]    = "autarco_house_power"; // publish	
	static constexpr char MQTT_AUTARCO_HYBRID_TOTAL_KWH_TOPIC[]   = "autarco_kwh"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_TOTAL_BATTERY_KWH_TOPIC[]   = "autarco_battery_kwh"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_TOTAL_BATTERY_DIS_KWH_TOPIC[]   = "autarco_battery_dis_kwh"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_TOTAL_FROM_GRID_KWH_TOPIC[]   = "autarco_from_grid_kwh"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_TOTAL_TO_GRID_KWH_TOPIC[]   = "autarco_to_grid_kwh"; // publish	
	static constexpr char MQTT_AUTARCO_HYBRID_DAY_KWH_TOPIC[]     = "autarco_day_kwh"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_DAY_BATTERY_KWH_TOPIC[]   = "autarco_day_battery_kwh"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_DAY_BATTERY_DIS_KWH_TOPIC[]   = "autarco_day_battery_dis_kwh"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_DAY_FROM_GRID_KWH_TOPIC[]   = "autarco_day_from_grid_kwh"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_DAY_TO_GRID_KWH_TOPIC[]   = "autarco_day_to_grid_kwh"; // publish	
	static constexpr char MQTT_AUTARCO_HYBRID_TEMP_TOPIC[]        = "autarco_temp"; // publish	
	static constexpr char MQTT_AUTARCO_HYBRID_DC1_VOLT_TOPIC[]    = "autarco_dc1_v"; // publish	
	static constexpr char MQTT_AUTARCO_HYBRID_DC1_CURR_TOPIC[]    = "autarco_dc1_a"; // publish	
	static constexpr char MQTT_AUTARCO_HYBRID_DC2_VOLT_TOPIC[]    = "autarco_dc2_v"; // publish	
	static constexpr char MQTT_AUTARCO_HYBRID_DC2_CURR_TOPIC[]    = "autarco_dc2_a"; // publish	
	static constexpr char MQTT_AUTARCO_HYBRID_DC3_VOLT_TOPIC[]    = "autarco_dc3_v"; // publish	
	static constexpr char MQTT_AUTARCO_HYBRID_DC3_CURR_TOPIC[]    = "autarco_dc3_a"; // publish
    static constexpr char MQTT_AUTARCO_HYBRID_BATTERY_CURR_TOPIC[]   = "autarco_bat_curr"; //publish and still missing in cpp
    static constexpr char MQTT_AUTARCO_HYBRID_BATTERY_CURR_BMS_TOPIC[]   = "autarco_bat_curr_bms"; //publish and still missing in cpp
    static constexpr char MQTT_AUTARCO_HYBRID_CHAR_DISCHAR_TOPIC[]   = "autarco_char_dischar"; //publish and still missing in cpp, defines if battery current pos or neg
    static constexpr char MQTT_AUTARCO_HYBRID_BATTERY_VOLT_TOPIC[]   = "autarco_bat_volt"; //publish and still missing in cpp
    static constexpr char MQTT_AUTARCO_HYBRID_BATTERY_VOLT_BMS_TOPIC[]   = "autarco_bat_volt_BMS"; //publish and still missing in cpp
    static constexpr char MQTT_AUTARCO_HYBRID_SOC_TOPIC[]   = "autarco_soc"; //publish
    static constexpr char MQTT_AUTARCO_HYBRID_SOH_TOPIC[]   = "autarco_soh"; //publish

    	


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
	



	class autarco_hybrid : public capability {
public:
		autarco_hybrid();
		~autarco_hybrid();
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
		mqtt_parameter_s32 m_power;
        mqtt_parameter_16 m_dc_power;
        mqtt_parameter_s32 m_battery_power;
		mqtt_parameter_s32 m_meter_power;
        mqtt_parameter_16 m_house_load;
        mqtt_parameter_s32 m_ac_grid_port_power;
        mqtt_parameter_16 m_soh;
        mqtt_parameter_16 m_soc;
		mqtt_parameter_float m_dc1_volt;
		mqtt_parameter_float m_dc1_curr;
		mqtt_parameter_float m_dc2_volt;
		mqtt_parameter_float m_dc2_curr;
		mqtt_parameter_float m_dc3_volt;
		mqtt_parameter_float m_dc3_curr;
		mqtt_parameter_32 m_last_total_kwh;
        mqtt_parameter_32 m_last_total_battery_kwh;
        mqtt_parameter_32 m_last_total_battery_dis_kwh;
        mqtt_parameter_32 m_last_total_from_grid_kwh;
        mqtt_parameter_32 m_last_total_to_grid_kwh;
		mqtt_parameter_float m_today_kwh;
        mqtt_parameter_float m_today_battery_kwh;
        mqtt_parameter_float m_today_battery_dis_kwh;
        mqtt_parameter_float m_today_from_grid_kwh;
        mqtt_parameter_float m_today_to_grid_kwh;
		mqtt_parameter_float m_total_kwh;
        mqtt_parameter_float m_total_battery_kwh;
        mqtt_parameter_float m_total_battery_dis_kwh;
        mqtt_parameter_float m_total_from_grid_kwh;
        mqtt_parameter_float m_total_to_grid_kwh;
		mqtt_parameter_float m_temp;
        mqtt_parameter_float m_bat_curr_bms;
        mqtt_parameter_float m_bat_curr;
        mqtt_parameter_16 m_char_dischar;
        mqtt_parameter_float m_bat_volt;
        mqtt_parameter_float m_bat_volt_bms;


		char m_buffer[AUTARCO_MAX_BUFFER]; // 80 byte buffer max
		uint8_t m_slot;
		float m_kwh_sync;
        float m_battery_dis_kwh_sync;
        float m_battery_kwh_sync;
        float m_from_grid_kwh_sync;
        float m_to_grid_kwh_sync;
		bool m_discovery_pub;
		uint8_t m_register_block;
		uint8_t m_parser_state;
		uint8_t m_payload_len;
		uint8_t m_payload_i;
		uint16_t m_crc;
		uint16_t m_req_start_addr;
		uint32_t m_last_char_in;
		SoftwareSerial * swSer1;
		uint8_t m_timeout;
        
        
	};


#endif // ifndef autarco_hybrid_h
