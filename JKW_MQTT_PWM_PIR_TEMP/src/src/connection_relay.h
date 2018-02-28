#ifndef connection_relay_h
	#define connection_relay_h

#include <ESP8266WiFi.h>

	#define AP_SSID                     "ESP_relay"
	#define AP_PW                       "ESP_relay_pw"
	#define MESH_PORT                   1883
	#define ESP8266_NUM_CLIENTS         5
	#define MAX_MSG_QUEUE               99
	#define COMM_TIMEOUT                140
	#define BLACKLIST_RECOVER_TIME_SEC  60 * 60 * 6

	#define CONNECTION_NOT_CONNECTED    0
	#define CONNECTION_DIRECT_CONNECTED 1
	#define CONNECTION_MESH_CONNECTED   2

	#define MSG_TYPE_SUBSCRPTION        1
	#define MSG_TYPE_PUBLISH            2
	#define MSG_TYPE_NW_LOOP_CHECK      3
	#define MSG_TYPE_ACK                4
	#define MSG_TYPE_ROUTING            5
	#define MSG_TYPE_PING               6

	#define OTA_STATUS_OK               0
	#define OTA_STATUS_WRITE_FAILED     1
	#define OTA_STATUS_SEQ_FAILED       2
	#define OTA_STATUS_START_FAILED     3
	#define OTA_STATUS_NOT_ENOUGH_SPACE 4
	#define OTA_STATUS_END_FAILED       5
	#define OTA_STATUS_UNKNOWN_CMD      6

	#define MQTT_OTA_BEGIN 'A'
	#define MQTT_OTA_WRITE 'B'
	#define MQTT_OTA_END 'C'




	class blacklist_entry {
public:
		blacklist_entry(uint8_t * MAC);
		~blacklist_entry();
		uint8_t get_fails(uint8_t * MAC);
		void set_fails(uint8_t * MAC, uint8_t v);
private:
		uint8_t m_mac[6];
		uint8_t m_fails;
		blacklist_entry * m_next;
		uint32_t m_last_change;
	};

	class connection_relay {
public:
		connection_relay();
		~connection_relay();
		bool MeshConnect();
		bool DirectConnect();
		bool connectServer(char * dev_short, char * login, char * pw);
		void disconnectServer();
		void startAP();
		void stopAP();
		bool subscribe(char * topic);
		bool publish(char * topic, char * msg);
		bool broadcast_publish_down(char * topic, char * mqtt_msg, uint16_t payload_size);
		bool connected();
		void receive_loop();
		bool loopCheck();
		bool publishRouting();
		uint8_t m_connection_type;
		uint8_t mqtt_ota(uint8_t * data, uint16_t size);

private:
		bool subscribe(char * topic, bool enqueue);
		bool publish(char * topic, char * mqtt_msg, bool enqueue);

		int8_t scan(bool blocking);
		bool send_up(char * msg, uint16_t size);
		bool enqueue_up(char * msg, uint16_t size);
		bool dequeue_up();

		void onClient(WiFiClient * c);
		void onData(WiFiClient * c, uint8_t client_nr);

		bool m_AP_running;
		WiFiServer * espServer;
		WiFiClient * espClients[ESP8266_NUM_CLIENTS]     = { 0 };
		uint32_t espClientsLastComm[ESP8266_NUM_CLIENTS] = { 0 };
		uint32_t espLastcomm;
		WiFiClient espUplink;
		uint8_t * outBuf[MAX_MSG_QUEUE];
		blacklist_entry * bl;

		uint32_t m_ota_bytes_in;
		uint16_t m_ota_seq;
		uint32_t m_ota_total_update_size;
		uint8_t m_ota_status;
	};


#include "main.h"

#endif // ifndef connection_relay_h
