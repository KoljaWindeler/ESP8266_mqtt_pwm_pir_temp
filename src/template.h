#ifdef NEVER_COMPILE_ME
#ifndef template_h
#define template_h

#include "main.h"

static constexpr char MQTT_TEMPLATE_TOPIC[]   = "topic1";


class template : public capability {
	public:
		template();
		~template();
		bool init();
		bool parse(uint8_t* config);
		uint8_t* get_key();

		//bool loop();

		//uint8_t count_intervall_update();
		//bool intervall_update(uint8_t slot);

		//bool subscribe();
		//bool receive(uint8_t* p_topic, uint8_t* p_payload);
		//bool publish();

		//uint8_t* get_dep();
		//bool reg_provider(peripheral * p, uint8_t *source);
	private:
};


#endif
#endif
