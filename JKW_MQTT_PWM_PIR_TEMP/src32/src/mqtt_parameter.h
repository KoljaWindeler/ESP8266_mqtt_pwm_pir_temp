#ifndef mqttp_h
#define mqttp_h
#include <Arduino.h>

#ifdef ESP32
  #include <WiFi.h>
	#include "../lib/EEPROM/EEPROM32.h"
#else
	#include <EEPROM.h>
  #include <ESP8266WiFi.h>
#endif

#define CHK_FORMAT_V4       0x44
#define LEN_FORMAT_V4				200

// Buffer to hold data from the WiFi manager for mqtt login
struct mqtt_data { //
	char login[16];
  char pw[16];
  char dev_short[50];
  char server_ip[16];
  char server_port[6];
  char nw_ssid[16];
  char nw_pw[16];
	char cap[64]; // capability
};

class mqtt_storage {
	public:
		mqtt_storage();
		void          explainMqttStruct(uint8_t i,boolean rn);
		void          explainFullMqttStruct(mqtt_data *mqtt);
		boolean       storeMqttStruct(char* temp,uint8_t size);
		boolean 			storeMqttStruct_universal(char * temp, uint8_t size, uint8_t chk);
		boolean       loadMqttStruct(char* temp,uint8_t size);
		boolean 			loadMqttStruct_universal(char * p_mqtt, uint8_t size, uint8_t chk);
		void          setMqtt(mqtt_data *mqtt);
		void 					loop();
		void 					init();
	private:
		char*					getMQTTelement(uint8_t i,mqtt_data * mqtt);
		mqtt_data     *m_mqtt;
    uint8_t       m_mqtt_sizes[8];
		uint8_t 			char_buffer;
		uint8_t * 		p;
		uint8_t 			f_p;
		uint8_t 			f_start;
		uint8_t 			skip_last;
};

class mqtt_parameter_8 {
	public:
		mqtt_parameter_8();
		void check_set(uint8_t input);
		void set(uint8_t input);
		void set(uint8_t input, bool update);
		uint8_t get_value();
		void outdated();
		void outdated(bool update_required);
		bool get_outdated();
	private:
		uint8_t _value;
		boolean _update_required;

};

class mqtt_parameter_16 {
	public:
		mqtt_parameter_16();
		void check_set(uint16_t input);
		void set(uint16_t input);
		void set(uint16_t input, bool update);
		uint16_t get_value();
		void outdated();
		void outdated(bool update_required);
		bool get_outdated();
	private:
		uint16_t _value;
		boolean _update_required;
};

class mqtt_parameter_32 {
	public:
		mqtt_parameter_32();
		void check_set(uint32_t input);
		void set(uint32_t input);
		void set(uint32_t input, bool update);
		uint32_t get_value();
		void outdated();
		void outdated(bool update_required);
		bool get_outdated();
	private:
		uint32_t _value;
		boolean _update_required;
};

#endif
