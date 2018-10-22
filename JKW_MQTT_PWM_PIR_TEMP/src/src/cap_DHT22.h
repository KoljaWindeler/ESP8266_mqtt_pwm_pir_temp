#ifndef J_DHT22_h
#define J_DHT22_h

#include "main.h"
#include <DHT.h>
#define DHT_PIN          2  // D4

class J_DHT22 : public peripheral {
	public:
		J_DHT22();
		~J_DHT22();
		bool init();
		// bool loop();
		bool intervall_update(uint8_t slot);
		// bool subscribe();
		uint8_t count_intervall_update();
		// bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool parse(uint8_t* config);
		uint8_t* get_key();
		// bool publish();
	private:
		uint8_t key[4];
};


#endif
