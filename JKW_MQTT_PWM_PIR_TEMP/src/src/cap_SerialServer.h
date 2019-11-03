#ifndef SerialServer_h
#define SerialServer_h

#include "main.h"
#include "../lib/softserial/SoftwareSerial.h"

class SerialServer : public capability {
	public:
		SerialServer();
		~SerialServer();
		bool init();
		bool parse(uint8_t* config);
		uint8_t* get_key();
		bool loop();

		//uint8_t count_intervall_update();
		//bool intervall_update(uint8_t slot);

		//bool subscribe();
		//bool receive(uint8_t* p_topic, uint8_t* p_payload);
		//bool publish();

		//uint8_t* get_dep();
		//bool reg_provider(peripheral * p, uint8_t *source);
	private:
		WiFiServer *ser2netServer;
		WiFiClient ser2netClient;
		SoftwareSerial * swSer1;
		uint16_t m_port;
		uint8_t m_rxPin;
		uint8_t m_txPin;
		uint8_t m_bufferSize;

};

#endif
