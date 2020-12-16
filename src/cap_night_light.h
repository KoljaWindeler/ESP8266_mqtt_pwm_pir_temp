#ifndef night_light_h
#define night_light_h

#include "main.h"
#define PIN_NIGHT_LIGHT 13

class night_light : public capability {
	public:
		night_light();
		~night_light();
		bool init();
		// bool loop();
		// bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		void interrupt();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		uint8_t* get_key();
	private:
		// bool publish();
};


#endif
