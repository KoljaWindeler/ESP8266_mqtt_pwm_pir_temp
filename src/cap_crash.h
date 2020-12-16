#ifndef crash_h
#define crash_h

#include "main.h"
#include "EspSaveCrash.h"

static constexpr char MQTT_CRASH_TOPIC[]   = "crash";


class crash : public capability {
	public:
		crash();
		~crash();
		bool init();
		bool parse(uint8_t* config);
		uint8_t* get_key();

		//bool loop();

		//uint8_t count_intervall_update();
		//bool intervall_update(uint8_t slot);

		//bool subscribe();
		//bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();

		//uint8_t* get_dep();
		//bool reg_provider(peripheral * p, uint8_t *source);
	private:
		EspSaveCrash *p_crash;
		bool m_published;
};


#endif
