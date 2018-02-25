#ifndef connection_relay_h
#define connection_relay_h


#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>


#define AP_SSID "ESP_relay"
#define AP_PW "ESP_relay_pw"
#define MESH_PORT 1883
#define ESP8266_NUM_CLIENTS 5
#define MAX_MSG_QUEUE 99
#define COMM_TIMEOUT 140
#define BLACKLIST_RECOVER_TIME_SEC 60*60*6

#define CONNECTION_NOT_CONNECTED 0
#define CONNECTION_DIRECT_CONNECTED 1
#define CONNECTION_MESH_CONNECTED 2

#define MSG_TYPE_SUBSCRPTION 1
#define MSG_TYPE_PUBLISH 2
#define MSG_TYPE_NW_LOOP_CHECK 3
#define MSG_TYPE_ACK 4
#define MSG_TYPE_ROUTING 5
#define MSG_TYPE_PING 6

class blacklist_entry {
public:
	blacklist_entry(uint8_t* MAC);
	~blacklist_entry();
	uint8_t get_fails(uint8_t* MAC);
	void set_fails(uint8_t* MAC, uint8_t v);
private:
	uint8_t m_mac[6];
	uint8_t m_fails;
	blacklist_entry* m_next;
	uint32_t m_last_change;
};

class connection_relay {
	public:
		connection_relay();
		~connection_relay();
		bool MeshConnect();
		bool DirectConnect();
		bool connectServer(char* dev_short, char* login, char* pw);
		void disconnectServer();
		void startAP();
		void stopAP();
		bool subscribe(char* topic);
		bool publish(char* topic, char* msg);
		bool broadcast_publish_down(char* topic, char* mqtt_msg);
		bool connected();
		void receive_loop();
		bool loopCheck();
		bool publishRouting();
		uint8_t m_connection_type;

	private:
		bool subscribe(char* topic, bool enqueue);
		bool publish(char* topic, char* mqtt_msg,bool enqueue);

 		int8_t scan(bool blocking);
		bool send_up(char* msg, uint16_t size);
		bool enqueue_up(char* msg, uint16_t size);
		bool dequeue_up();

		void onClient(WiFiClient* c);
		void onData(WiFiClient* c, uint8_t client_nr);

		bool m_AP_running;
		WiFiServer* espServer;
    WiFiClient* espClients[ESP8266_NUM_CLIENTS] = {0};
		uint32_t espClientsLastComm[ESP8266_NUM_CLIENTS] = {0};
		uint32_t espLastcomm;
		WiFiClient espUplink;
		uint8_t* outBuf[MAX_MSG_QUEUE];
		blacklist_entry* bl;
};


#include "main.h"

#endif
