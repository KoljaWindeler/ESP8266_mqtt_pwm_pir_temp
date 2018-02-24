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

#define CONNECTION_NOT_CONNECTED 0
#define CONNECTION_DIRECT_CONNECTED 1
#define CONNECTION_MESH_CONNECTED 2

#define MSG_TYPE_SUBSCRPTION 1
#define MSG_TYPE_PUBLISH 2
#define MSG_TYPE_NW_LOOP_CHECK 3
#define MSG_TYPE_ACK 4
#define MSG_TYPE_ROUTING 5
#define MSG_TYPE_PING 6


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
		uint8_t m_connection_type;

	private:
		bool subscribe(char* topic, bool enqueue);
		bool publish(char* topic, char* mqtt_msg,bool enqueue);

 		int8_t scan(bool blocking);
		bool send_up(char* msg, uint16_t size);
		bool enqueue_up(char* msg, uint16_t size);
		bool dequeue_up();

		void onClient(WiFiClient* c);
		void onData(WiFiClient* c);

		bool m_AP_running;
		WiFiServer* espServer;
    WiFiClient* espClients[ESP8266_NUM_CLIENTS] = {0};
		uint32_t espClientsLastComm[ESP8266_NUM_CLIENTS] = {0};
		uint32_t espLastcomm;
		WiFiClient espUplink;
		uint8_t* outBuf[MAX_MSG_QUEUE];
};

#include "main.h"

#endif
