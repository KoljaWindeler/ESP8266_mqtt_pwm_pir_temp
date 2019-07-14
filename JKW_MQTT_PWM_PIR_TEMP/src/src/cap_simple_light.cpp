#include <cap_simple_light.h>
#ifdef WITH_SL

simple_light::simple_light(){
	m_discovery_pub = false;
};

simple_light::~simple_light(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_SL_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing SL config "), COLOR_YELLOW);
		logger.pln(t);
		network.unsubscribe(t);
		network.publish(t,(char*)"");
		m_discovery_pub = false;
		delete[] t;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("Simple light deleted"), COLOR_YELLOW);
};

uint8_t* simple_light::get_key(){
	return (uint8_t*)"SL";
}

uint8_t* simple_light::get_dep(){
	return (uint8_t*)"LIG";
}

bool simple_light::parse(uint8_t* config){
	if(cap.parse(config,get_key(),get_dep())){
		m_pin = SIMPLE_LIGHT_PIN;
		return true;
	}
	// check for all pins with dedicated string
	else if(cap.parse_wide(config, get_key(),&m_pin, (uint8_t*)"LIG")){
		return true;
	}

	return false;
}

bool simple_light::init(){
	pinMode(m_pin, OUTPUT);
	logger.print(TOPIC_GENERIC_INFO, F("Simple light init "), COLOR_GREEN);
	sprintf(m_msg_buffer,"GPIO %i",m_pin);
	logger.pln(m_msg_buffer);
	return true;
	//setState();
}

// bool simple_light::loop(){
// 	return false;
// }
//
// bool simple_light::intervall_update(uint8_t slot){
// 	return false;
// }
//
// bool simple_light::subscribe(){
// 	return true;
// }
//
// bool simple_light::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }
//
//
bool simple_light::publish(){
#ifdef WITH_DISCOVERY

	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_SL_TOPIC); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_SL_MSG)+3*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_SL_MSG, mqtt.dev_short, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("SL discovery"), COLOR_GREEN);
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
	return false;
}

// function called to adapt the state of the led
void simple_light::setColor(uint8_t r, uint8_t g, uint8_t b){
	m_state.set(r);
	if (r) {
		digitalWrite(m_pin, HIGH);
		logger.println(TOPIC_INFO_SL, F("Simple light on"), COLOR_PURPLE);
	} else {
		digitalWrite(m_pin, LOW);
		logger.println(TOPIC_INFO_SL, F("Simple light off"), COLOR_PURPLE);
	}
}
#endif
