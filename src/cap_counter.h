#ifndef counter_h
#define counter_h

#include "main.h"

    static constexpr char MQTT_COUNTER_TOPIC[]   = "counter";

    #ifdef WITH_DISCOVERY
    static constexpr char MQTT_DISCOVERY_COUNTER_TOPIC[]      = "homeassistant/sensor/%s_counter/config";
    static constexpr char MQTT_DISCOVERY_COUNTER_MSG[]      = "{\"name\":\"%s_counter\", \"stat_t\": \"%s/r/counter\"}";
    #endif


class counter : public capability {
	public:
		counter();
		~counter();
		bool init();
		bool parse(uint8_t* config);
		uint8_t* get_key();

		bool loop();

		//uint8_t count_intervall_update();
		//bool intervall_update(uint8_t slot);

		bool subscribe();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();

		//uint8_t* get_dep();
		//bool reg_provider(peripheral * p, uint8_t *source);
	private:
        bool m_discovery_pub;

		uint16_t m_ghost;
        uint8_t m_pin;
        mqtt_parameter_32 m_state;

		uint8_t m_buttonState;             // the current reading from the input pin
		uint8_t m_lastButtonState = LOW;   // the previous reading from the input pin

		// the following variables are unsigned longs because the time, measured in
		// milliseconds, will quickly become a bigger number than can be stored in an int.
		uint32_t m_lastDebounceTime = 0;  // the last time the output pin was toggled
};


#endif
