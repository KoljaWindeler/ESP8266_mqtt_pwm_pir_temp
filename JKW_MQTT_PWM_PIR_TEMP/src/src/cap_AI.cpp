#include <cap_AI.h>

#ifdef WITH_AI


AI::AI(){
	m_discovery_pub = false;
};

AI::~AI(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_DIMM_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing AI config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		m_discovery_pub = false;
		delete[] t;
	}
#endif

	logger.println(TOPIC_GENERIC_INFO, F("AI deleted"), COLOR_YELLOW);
};

bool AI::parse(uint8_t* config){
	return cap.parse(config,get_key(),get_dep());
}


uint8_t* AI::get_dep(){
	return (uint8_t*)"LIG";
}

my92x1* AI::getmy929x1(){
	return &_my92x1;
}

uint8_t* AI::get_key(){
	return (uint8_t*)"AI";
}

bool AI::init(){
	_my92x1 = my92x1();
	_my92x1.init(false,AI_DI_PIN, AI_DCKI_PIN, 4); // false = AI
	logger.println(TOPIC_GENERIC_INFO, F("AI init"), COLOR_GREEN);
	return true;
}


// bool AI::loop(){
// 	return false; // i did nothing
// }


void AI::setColor(uint8_t r, uint8_t g, uint8_t b){
	uint8_t w=0;
	if(r==b && b==g){
		w=r;
		r=0;
		g=0;
		b=0;
	}
	_my92x1.setColor((my92x1_color_t) { r, g, b, w, 0 });          // last two: warm white, cold white
}

uint8_t AI::getState(led* color){
	color->r = m_light_current.r;
	color->g = m_light_current.g;
	color->b = m_light_current.b;
	return m_state.get_value();
}

// bool AI::subscribe(){
// 	return true;
// }
//
// bool AI::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }
//
//
 bool AI::publish(){
	 // function called to publish the state of the PWM (on/off)
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_DIMM_TOPIC); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_DIMM_MSG)+5*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_DIMM_MSG, mqtt.dev_short, mqtt.dev_short, mqtt.dev_short, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("AI discovery"), COLOR_GREEN);
			//logger.p(t);
			//logger.p(" -> ");
			//logger.pln(m);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
			return true;
		}
	}
#endif
 	return false; // i did notihgin
 }

#endif
