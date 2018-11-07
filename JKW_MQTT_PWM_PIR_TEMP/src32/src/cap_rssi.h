#ifndef rssi_h
#define rssi_h

#include "main.h"
static constexpr char MQTT_RSSI_TOPIC[]          = "rssi";        // publish

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
		// bool publish();
	private:
		int getRSSIasQuality(int RSSI);
		mqtt_parameter_8 m_state;
};


#endif
