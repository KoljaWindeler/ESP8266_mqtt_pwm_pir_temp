#ifndef rssi_h
#define rssi_h

#include "main.h"

class rssi : public capability {
	public:
		rssi();
		~rssi();
		bool init();
		// bool loop();
		bool intervall_update(uint8_t slot);
		// bool subscribe();
		uint8_t count_intervall_update();
		bool parse(uint8_t* config);
		// bool receive(uint8_t* p_topic, uint8_t* p_payload);
		uint8_t* get_key();
		bool publish();
	private:
		mqtt_parameter_8 m_state;
		bool m_discovery_pub;
};


#endif
