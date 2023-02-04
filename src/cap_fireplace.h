#ifdef WITH_FIREPLACE
#ifndef fireplace_h
#define fireplace_h

#include "main.h"
#include "cap_DS.h"
#include "cap_ebus.h"

static constexpr char MQTT_TEMPLATE_TOPIC[]   = "topic1";


class fireplace : public capability {
	public:
		fireplace();
		~fireplace();
		bool init();
		bool parse(uint8_t* config);
		uint8_t* get_key();

		bool loop();
		bool emergency_loop();	

		//uint8_t count_intervall_update();
		//bool intervall_update(uint8_t slot);

		//bool subscribe();
		//bool receive(uint8_t* p_topic, uint8_t* p_payload);
		//bool publish();

		//uint8_t* get_dep();
		//bool reg_provider(peripheral * p, uint8_t *source);
	private:
		bool sub_loop(bool emergency);
		
};


#endif
#endif
