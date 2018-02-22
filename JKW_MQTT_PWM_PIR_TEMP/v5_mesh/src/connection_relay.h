#ifndef connection_relay_h
#define connection_relay_h


#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>


#define AP_SSID "ESP_relay"
#define AP_PW "ESP_relay_pw"
#define MESH_PORT 1883
#define ESP8266_NUM_CLIENTS 99
#define MAX_MSG_QUEUE 99

#define CONNECTION_NOT_CONNECTED 0
#define CONNECTION_DIRECT_CONNECTED 1
#define CONNECTION_MESH_CONNECTED 2

#define MSG_TYPE_SUBSCRPTION 1
#define MSG_TYPE_PUBLISH 2
#define MSG_TYPE_NW_LOOP_CHECK 3
#define MSG_TYPE_ACK 4


class connection_relay {
	public:
		connection_relay();
		~connection_relay();
		void begin();
		bool MeshConnect();
		bool DirectConnect();
		bool connectServer(char* dev_short, char* login, char* pw);
		void disconnectServer();
		void startAP();
		void stopAP();
		bool subscribe(char* topic);
		bool subscribe(char* topic, bool enqueue);
		bool publish(char* topic, char* msg);
		bool publish(char* topic, char* mqtt_msg,bool enqueue);
		bool broadcast_publish_down(char* topic, char* mqtt_msg);
		bool connected();
		void receive_loop();
		bool loopCheck();
		uint8_t m_connection_type;

	private:
 		int8_t scan(bool blocking);
		bool send_up(char* msg);
		bool enqueue_up(char* msg);
		bool dequeue_up();

		void onClient(AsyncClient* c);
		void onDisconnect(AsyncClient* c);
		void onTimeout(AsyncClient* c, uint32_t time);
		void onData(AsyncClient* c, void* data, size_t len);
		void onServerData(AsyncClient* c, void* data, size_t len);
		void onClientDisconnect(const WiFiEventSoftAPModeStationDisconnected* ip);
		void onClientConnect(const WiFiEventSoftAPModeStationConnected* ip);
		void onError(AsyncClient* c, int8_t error);

		bool m_AP_running;
		AsyncServer* espServer;
    AsyncClient* espClients[ESP8266_NUM_CLIENTS] = {0};
		WiFiClient espUplink;
		uint8_t* outBuf[MAX_MSG_QUEUE];
};

#include "main.h"

#endif
